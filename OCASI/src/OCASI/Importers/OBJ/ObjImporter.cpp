#include "ObjImporter.h"

#include "OCASI/Importers/OBJ/FileParser.h"
#include "OCASI/Importers/OBJ/MtlParser.h"

#include <fstream>

namespace OCASI {
    struct VertexIndices
    {
        size_t VertexIndex;
        size_t TextureCoordinateIndex;
        size_t NormalIndex;

        bool operator==(const VertexIndices& other) const
        {
            return VertexIndex == other.VertexIndex && TextureCoordinateIndex == other.TextureCoordinateIndex && NormalIndex == other.NormalIndex;
        }
    };
}

namespace std {

    template <>
    struct std::hash<OCASI::VertexIndices>
    {
        std::size_t operator()(const OCASI::VertexIndices& v) const
        {
            return std::hash<size_t>()(v.VertexIndex) + std::hash<size_t>()(v.TextureCoordinateIndex) + std::hash<size_t>()(v.NormalIndex);
        }
    };
}

namespace OCASI {

    // OBJ files only support one set of texture coordinates
    constexpr uint8_t OBJ_TEXTURE_COORDINATE_ARRAY_INDEX = 0;

    ObjImporter::ObjImporter(FileReader& reader)
        : m_FileReader(reader)
    {
    }

    std::shared_ptr<Scene> ObjImporter::Load3DFile()
    {
        OBJ::FileParser objParser(m_FileReader);
        m_OBJModel = objParser.ParseOBJFile();

        if (!m_OBJModel)
            return nullptr;

        Path folder = m_FileReader.GetParentPath();

        if(!m_OBJModel->MTLFilePath.empty())
        {

            OBJ::MtlParser mtlParser(m_OBJModel, folder / m_OBJModel->MTLFilePath);

            if (!mtlParser.ParseMTLFile())
                return nullptr;
        }

        return ConvertToOCASIScene(folder);
    }

    bool ObjImporter::CanLoad()
    {
        return Util::FindTokensInFirst100Lines(m_FileReader, { "v", "vn", "vt", "mtlib", "f", "usemtl" });
    }

    std::shared_ptr<Scene> ObjImporter::ConvertToOCASIScene(const Path& folder)
    {
        m_OutputScene = std::make_shared<Scene>();

        // Converting from a RHCS to a LHCS
        for (glm::vec3& v : m_OBJModel->Vertices)
            v = { v.x, v.y, -v.z };

        for (glm::vec3& n : m_OBJModel->Normals)
            n = { n.x, n.y, -n.z };

        /// 3D Model conversion

        for (const OBJ::Mesh& m : m_OBJModel->Meshes)
        {
            Model& newModel = m_OutputScene->Models.emplace_back();
            newModel.Name = m.Name;
            newModel.FaceType = m.FaceType;
            newModel.Dimension = m.Dimension;

            Mesh& newMesh = newModel.Meshes.emplace_back();

            // Indices creation
            std::unordered_map<VertexIndices, size_t> lookUpTable;
            for (OBJ::Face f : m.Faces)
            {
                for (size_t i = 0; i < (size_t) f.Type; i++)
                {
                    VertexIndices indices = { f.VertexIndices.at(i),
                                              !f.TextureCoordinateIndices.empty() ? f.TextureCoordinateIndices.at(i) : INVALID_ID,
                                              !f.NormalIndices.empty() ? f.NormalIndices.at(i) : INVALID_ID };

                    if (lookUpTable.find(indices) == lookUpTable.end())
                    {
                        CreateNewVertex(newMesh, indices);
                        lookUpTable[indices] = newMesh.Indices.at(newMesh.Vertices.size() - 1);
                    }
                    else
                    {
                        newMesh.Indices.push_back(lookUpTable.at(indices));
                    }
                }
            }
        }

        /// Material conversion

        for (const auto& [name, mat] : m_OBJModel->Materials)
        {
            Material& newMat = m_OutputScene->Materials.emplace_back();
            newMat.Name = mat.Name;

            if (mat.PBRExtension.has_value())
            {
                const OBJ::PBRMaterialExtension& pbrExtension = mat.PBRExtension.value();
                newMat.PbrMaterial = std::make_unique<PBRMaterial>();

                newMat.PbrMaterial->AlbedoColour = pbrExtension.AlbedoColour;
                newMat.PbrMaterial->Roughness = pbrExtension.Roughness;
                newMat.PbrMaterial->Metallic = pbrExtension.Metallic;
                newMat.PbrMaterial->IOR = pbrExtension.IOR;
                newMat.PbrMaterial->Anisotropy = pbrExtension.Anisotropy;
                newMat.PbrMaterial->AnisotropyRotation = pbrExtension.AnisotropyRotation;
                newMat.PbrMaterial->Clearcoat = pbrExtension.Clearcoat;
                newMat.PbrMaterial->ClearcoatRoughness = pbrExtension.ClearcoatRoughness;
            }
            else
            {
                newMat.BlinnPhongMaterial = std::make_unique<BlinnPhongMaterial>();

                newMat.BlinnPhongMaterial->DiffuseColour = mat.DiffuseColour;
                newMat.BlinnPhongMaterial->SpecularColour = mat.SpecularColour;
                newMat.BlinnPhongMaterial->Shininess = mat.Shininess;
                newMat.BlinnPhongMaterial->Transparency = mat.Opacity;
                newMat.BlinnPhongMaterial->EmissiveColour = mat.EmissiveColour;
            }

            for (uint32_t i = 0; i < OBJ::MAX_NON_PBR_TEXTURE_COUNT + OBJ::MAX_PBR_TEXTURE_COUNT; i++)
            {
                SortTextures(newMat, mat, folder, i);
            }
        }

        if (!m_OBJModel->Objects.empty())
        {
            for (const OBJ::Object& o : m_OBJModel->Objects)
            {
                m_OutputScene->RootNodes.push_back(CreateNodesFromObject(o));
            }
        }
        else
        {
            for (int i = 0; i < m_OBJModel->Meshes.size(); i++)
            {
                m_OutputScene->RootNodes.push_back(CreateNodeFromMesh(i));
            }
        }

        return m_OutputScene;
    }

    std::shared_ptr<Node> ObjImporter::CreateNodesFromObject(const OBJ::Object& o) const
    {
        OBJ::Mesh& m = m_OBJModel->Meshes.at(o.Mesh);

        std::shared_ptr<Node> root = std::make_shared<Node>();
        root->Parent = nullptr;
        root->MeshIndex = o.Mesh;
        if (!m.MaterialName.empty())
        {
            for (int i = 0; i < m_OutputScene->Materials.size(); i++)
            {
                Material& mat = m_OutputScene->Materials.at(i);
                if (mat.Name == m.MaterialName)
                {
                    // Always the firs mesh as obj does not have multiple meshes in one model
                    m_OutputScene->Models.at(o.Mesh).Meshes.at(0).MaterialIndex = i;
                }
            }
        }

        for(uint32_t m : o.Children)
        {
            std::shared_ptr<Node> child = std::make_unique<Node>();
            child->Parent = root;
            child->MeshIndex = m;
            root->Children.push_back(child);
        }
        return root;
    }

    std::shared_ptr<Node> ObjImporter::CreateNodeFromMesh(uint32_t mesh)
    {
        OBJ::Mesh& m = m_OBJModel->Meshes.at(mesh);

        std::shared_ptr<Node> newNode = std::make_unique<Node>();
        newNode->Parent = nullptr;
        newNode->Children.clear();
        newNode->MeshIndex = mesh;
        if (!m.MaterialName.empty())
        {
            for (int i = 0; i < m_OutputScene->Materials.size(); i++)
            {
                if (m_OutputScene->Materials.at(i).Name == m.MaterialName)
                {
                    // Always the firs mesh as obj does not have multiple meshes in one model
                    m_OutputScene->Models.at(mesh).Meshes.at(0).MaterialIndex = i;
                }
            }
        }
        return newNode;
    }

    void ObjImporter::CreateNewVertex(Mesh &model, const VertexIndices& indices) const
    {
        model.Vertices.push_back(m_OBJModel->Vertices.at(indices.VertexIndex));

        if (indices.TextureCoordinateIndex != INVALID_ID)
            model.TexCoords[OBJ_TEXTURE_COORDINATE_ARRAY_INDEX].push_back(m_OBJModel->TexCoords.at(indices.TextureCoordinateIndex));
        if (indices.NormalIndex != INVALID_ID)
            model.Normals.push_back(m_OBJModel->Normals.at(indices.NormalIndex));

        model.Indices.push_back(model.Vertices.size() - 1);
    }

    void ObjImporter::SortTextures(Material &newMat, const OBJ::Material &mat, const Path& folder, uint32_t i)
    {
        // This value is needed to convert the OBJ::TextureType to a OCASI::TextureOrientation
        // for reflection textures. In OBJ each side of the cube map is provided using a single
        // image. The value 6 is just the mapping value that needs to be subtracted from the TextureType
        // to be able to cast the integer value of the enum to the integer value of the TextureOrientation.
        const uint8_t REFLECTION_TEXTURE_INDEX_NORMALIZER = 6;

        OBJ::TextureType type = (OBJ::TextureType) i;
        std::string texturePath;
        bool clampValue = false;
        if (!OBJ::HasPBRTextureType(type))
        {
            texturePath = mat.Textures.at(type);
            clampValue = mat.TextureClamps.at(i);
        }
        else if (OBJ::HasPBRTextureType(type) && mat.PBRExtension.has_value())
        {
            texturePath = mat.PBRExtension->Textures.at(type);
            clampValue = mat.PBRExtension->TextureClamps.at(i);
        }

        if (texturePath.empty())
            return;

        ImageSettings settings = {};
        settings.Clamp = clampValue ? ClampOption::ClampToBorder : ClampOption::ClampRepeat;

        if (mat.PBRExtension.has_value())
        {
            PBRMaterial& pbrMaterial = *newMat.PbrMaterial;
            switch (type)
            {
                case OBJ::TextureType::Albedo:
                    pbrMaterial.AlbedoTexture = std::make_unique<Image>(folder / texturePath, settings);
                case OBJ::TextureType::Roughness:
                    pbrMaterial.RoughnessTexture = std::make_unique<Image>(folder / texturePath, settings);
                case OBJ::TextureType::Metallic:
                    pbrMaterial.MetallicTexture = std::make_unique<Image>(folder / texturePath, settings);
                case OBJ::TextureType::Sheen:
                    // TODO: Not Implemented
                case OBJ::TextureType::Clearcoat:
                    pbrMaterial.ClearcoatTexture = std::make_unique<Image>(folder / texturePath, settings);
                case OBJ::TextureType::ClearcoatRoughness:
                    pbrMaterial.ClearcoatRoughnessTexture = std::make_unique<Image>(folder / texturePath, settings);
                case OBJ::TextureType::Occlusion:
                    pbrMaterial.AmbientOcclusionTexture = std::make_unique<Image>(folder / texturePath, settings);
                case OBJ::TextureType::NormalPBR:
                    pbrMaterial.NormalMap = std::make_unique<Image>(folder / texturePath, settings);
                default:
                    OCASI_FAIL("Something went very wrong");
            }
        }
        else
        {
            BlinnPhongMaterial& blinnPhongMaterial = *newMat.BlinnPhongMaterial;

            switch (type)
            {
                case OBJ::TextureType::Diffuse:
                    blinnPhongMaterial.DiffuseTexture = std::make_unique<Image>(folder / texturePath, settings);
                case OBJ::TextureType::Ambient:
                    blinnPhongMaterial.AmbientTexture = std::make_unique<Image>(folder / texturePath, settings);
                case OBJ::TextureType::Specular:
                    blinnPhongMaterial.SpecularTexture = std::make_unique<Image>(folder / texturePath, settings);
                case OBJ::TextureType::Emissive:
                    blinnPhongMaterial.EmissiveTexture = std::make_unique<Image>(folder / texturePath, settings);
                case OBJ::TextureType::Transparency:
                    blinnPhongMaterial.TransparencyTexture = std::make_unique<Image>(folder / texturePath, settings);
                case OBJ::TextureType::Shininess:
                    blinnPhongMaterial.ShininessTexture = std::make_unique<Image>(folder / texturePath, settings);
                case OBJ::TextureType::Normal:
                    blinnPhongMaterial.NormalMap = std::make_unique<Image>(folder / texturePath, settings);

                case OBJ::TextureType::ReflectionTop:
                case OBJ::TextureType::ReflectionBottom:
                case OBJ::TextureType::ReflectionBack:
                case OBJ::TextureType::ReflectionFront:
                case OBJ::TextureType::ReflectionLeft:
                case OBJ::TextureType::ReflectionRight:
                case OBJ::TextureType::ReflectionSphere:
                    settings.Orientation = (TextureOrientation) (type - REFLECTION_TEXTURE_INDEX_NORMALIZER);
                    newMat.BlinnPhongMaterial->ReflectionMaps.push_back(std::make_unique<Image>(folder / texturePath, settings));
                    break;
            }
        }
    }
}
