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

        // Converting from a RHCS to LHCS
        for (glm::vec3& v : m_OBJModel->Vertices)
            v = { v.x, v.y, -v.z };

        for (glm::vec3& n : m_OBJModel->Normals)
            n = { n.x, n.y, -n.z };

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

            for (size_t i = 0; i < mat.Textures.size(); i++)
            {
                SortTextures(newMat, mat, folder, i);
            }
        }

        if (!m_OBJModel->RootObjects.empty())
        {
            for (const OBJ::Object& o : m_OBJModel->RootObjects)
            {
                m_OutputScene->RootNodes.push_back(CreateNodes(o));
            }
        }

        return m_OutputScene;
    }

    std::shared_ptr<Node> ObjImporter::CreateNodes(const OBJ::Object& o)
    {
        std::shared_ptr<Node> node = std::make_unique<Node>();
        node->Parent = nullptr;

        if (!o.Meshes.empty())
        {
            node->ModelIndex = m_OutputScene->Models.size();
            Model& model = m_OutputScene->Models.emplace_back();
            model.Name = o.Name;

            for (size_t i = 0; i < o.Meshes.size(); i++)
            {
                model.Meshes.push_back(CreateMesh(o.Meshes.at(i)));
            }
        }

        for (size_t meshIndex : o.Groups)
        {
            std::shared_ptr<Node> groupNode = std::make_unique<Node>();
            node->Children.push_back(groupNode);
            groupNode->Parent = node;
            groupNode->ModelIndex = m_OutputScene->Models.size();

            Model& groupModel = m_OutputScene->Models.emplace_back();
            groupModel.Name = m_OBJModel->Meshes.at(meshIndex).Name;
            groupModel.Meshes.push_back(CreateMesh(meshIndex));

        }

        return node;
    }

    Mesh ObjImporter::CreateMesh(size_t mesh) const
    {
        const OBJ::Mesh& m = m_OBJModel->Meshes.at(mesh);

        Mesh outMesh = {};
        outMesh.Name = m.Name;
        // In order to generate indices and remove duplicate vertices, we have to keep track of all unique VertexIndices.
        // This means that for every face the indices into the global vertex arrays (vertex array, normal array, texture
        // coordinate array) have to be checked against all already loaded indices. If there is a match, we just use the
        // index of that matching vertex in the indices array.
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
                    CreateNewVertex(outMesh, indices);
                    lookUpTable[indices] = outMesh.Indices.at(outMesh.Vertices.size() - 1);
                }
                else
                {
                    outMesh.Indices.push_back(lookUpTable.at(indices));
                }
            }
        }

        if (!m.MaterialName.empty())
        {
            for (size_t i = 0; i < m_OutputScene->Materials.size(); i++)
            {
                if (m_OutputScene->Materials.at(i).GetName() == outMesh.Name)
                    outMesh.MaterialIndex = i;
            }
        }

        outMesh.Dimension = m.Dimension;
        outMesh.FaceType = m.FaceType;

        return outMesh;
    }

    void ObjImporter::CreateNewVertex(Mesh& mesh, const VertexIndices& indices) const
    {
        mesh.Vertices.push_back(m_OBJModel->Vertices.at(indices.VertexIndex));
        if (!m_OBJModel->VertexColours.empty())
            mesh.VertexColours.push_back(m_OBJModel->VertexColours.at(indices.VertexIndex));

        if (indices.TextureCoordinateIndex != INVALID_ID)
            mesh.TexCoords[OBJ_TEXTURE_COORDINATE_ARRAY_INDEX].push_back(m_OBJModel->TexCoords.at(indices.TextureCoordinateIndex));
        if (indices.NormalIndex != INVALID_ID)
            mesh.Normals.push_back(m_OBJModel->Normals.at(indices.NormalIndex));

        mesh.Indices.push_back(mesh.Vertices.size() - 1);
    }

    void ObjImporter::SortTextures(Material &newMat, const OBJ::Material &mat, const Path& folder, size_t i)
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
