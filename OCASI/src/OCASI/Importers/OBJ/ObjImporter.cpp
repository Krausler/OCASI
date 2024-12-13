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
            newMat.SetName(mat.Name);

            if (mat.PBRExtension.has_value())
            {
                const OBJ::PBRMaterialExtension& pbrExtension = mat.PBRExtension.value();

                newMat.SetValue(MATERIAL_ROUGHNESS_INDEX, pbrExtension.Roughness);
                newMat.SetValue(MATERIAL_METALLIC_INDEX, pbrExtension.Metallic);
                newMat.SetValue(MATERIAL_IOR_INDEX, pbrExtension.IOR);
                newMat.SetValue(MATERIAL_ANISOTROPY_INDEX, pbrExtension.Anisotropy);
                newMat.SetValue(MATERIAL_ANISOTROPY_ROTATION_INDEX, pbrExtension.AnisotropyRotation);
                newMat.SetValue(MATERIAL_CLEARCOAT_INDEX, pbrExtension.Clearcoat);
                newMat.SetValue(MATERIAL_CLEARCOAT_ROUGHNESS_INDEX, pbrExtension.ClearcoatRoughness);
            }

            newMat.SetValue(MATERIAL_ALBEDO_COLOUR_INDEX, mat.DiffuseColour);
            newMat.SetValue(MATERIAL_SPECULAR_COLOUR_INDEX, mat.SpecularColour);
            newMat.SetValue(MATERIAL_SPECULAR_STRENGTH_INDEX, mat.Shininess);
            newMat.SetValue(MATERIAL_EMISSIVE_COLOUR_INDEX, mat.EmissiveColour);
            newMat.SetValue(MATERIAL_TRANSPARENCY_INDEX, mat.Opacity);

            for (uint32_t i = 0; i < mat.Textures.size(); i++)
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
                if (mat.GetName() == m.MaterialName)
                {
                    // Every model has only one submesh in the OBJ file format. There is support for nesting node structures,
                    // but not for multiple submeshes in one mesh
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
                if (m_OutputScene->Materials.at(i).GetName() == m.MaterialName)
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
        // image. The value 8 is just the mapping value that needs to be subtracted from the TextureType
        // to be able to cast the integer value of the enum to the integer value of the TextureOrientation.
        const uint8_t REFLECTION_TEXTURE_INDEX_NORMALIZER = 8;

        OBJ::TextureType type = (OBJ::TextureType) i;
        std::string texturePath = mat.Textures.at(type);

        if (texturePath.empty())
            return;

        ImageSettings settings = {};
        settings.Clamp = mat.TextureClamps.at(i) ? ClampOption::ClampToEdge : ClampOption::ClampRepeat;

        switch (type)
        {
            case OBJ::TextureType::Roughness:
            {
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_ROUGHNESS_INDEX, image);
                break;
            }
            case OBJ::TextureType::Metallic:
            {
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_METALLIC_INDEX, image);
                break;
            }
            case OBJ::TextureType::Sheen:
                // TODO: Not Implemented
                break;
            case OBJ::TextureType::Clearcoat:
            {
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_CLEARCOAT_INDEX, image);
                break;
            }
            case OBJ::TextureType::ClearcoatRoughness:
            {
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_CLEARCOAT_ROUGHNESS_INDEX, image);
                break;
            }
            case OBJ::TextureType::Occlusion:
            {
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_OCCLUSION_INDEX, image);
                break;
            }
            case OBJ::TextureType::Diffuse:
            {
                // Diffuse textures are classified as the objects base color
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_ALBEDO_INDEX, image);
                break;
            }
            case OBJ::TextureType::Ambient:
            {
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_AMBIENT_INDEX, image);
                break;
            }
            case OBJ::TextureType::Specular:
            {
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_SPECULAR_INDEX, image);
                break;
            }
            case OBJ::TextureType::Emissive:
            {
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_EMISSIVE_INDEX, image);
                break;
            }
            case OBJ::TextureType::Transparency:
            {
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_TRANSPARENCY_INDEX, image);
                break;
            }
            case OBJ::TextureType::Shininess:
            {
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_SPECULAR_STRENGTH_INDEX, image);
                break;
            }
            case OBJ::TextureType::Normal:
            {
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_NORMAL_INDEX, image);
                break;
            }
            case OBJ::TextureType::ReflectionTop:
            case OBJ::TextureType::ReflectionBottom:
            case OBJ::TextureType::ReflectionBack:
            case OBJ::TextureType::ReflectionFront:
            case OBJ::TextureType::ReflectionLeft:
            case OBJ::TextureType::ReflectionRight:
            case OBJ::TextureType::ReflectionSphere:
            {
                auto image = std::make_shared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_REFLECTION_MAP_TOP + type - REFLECTION_TEXTURE_INDEX_NORMALIZER, image);
                break;
            }
            default:
            OCASI_FAIL("Something went very wrong");
                break;
        }
    }
}
