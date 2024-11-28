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

        /// 3D Mesh conversion

        for (const OBJ::Mesh& m : m_OBJModel->Meshes)
        {
            Mesh& newMesh = m_OutputScene->Meshes.emplace_back();
            newMesh.Name = m.Name;
            newMesh.FaceType = m.FaceType;
            newMesh.Dimension = m.Dimension;

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

            newMesh.HasNormals = !newMesh.Normals.empty();
            newMesh.HasTexCoords = !newMesh.TexCoords.empty();
            // OBJ files do not support tangents
            newMesh.HasTangents = false;
        }

        /// Material conversion
        for (const auto& [name, mat] : m_OBJModel->Materials)
        {
            Material& newMat = m_OutputScene->Materials.emplace_back();
            newMat.Name = mat.Name;
            newMat.AlbedoColour = mat.AmbientColour;
            newMat.DiffuseColour = mat.DiffuseColour;
            newMat.SpecularColour = mat.SpecularColour;
            newMat.EmissiveColour = mat.EmissiveColour;
            newMat.Shininess = mat.Shininess;
            newMat.Transparency = mat.Opacity;
            newMat.Roughness = mat.Roughness;
            newMat.Metallic = mat.Metallic;
            newMat.IOR = mat.IOR;
            newMat.Sheen = mat.Sheen;
            newMat.Anisotropy = mat.Anisotropy;
            newMat.AnisotropyRotation = mat.AnisotropyRotation;
            newMat.Clearcoat = mat.Clearcoat;
            newMat.ClearcoatRoughness = mat.ClearcoatRoughness;
            newMat.BumpMultiplier = mat.BumpMapMultiplier;

            for (uint32_t i = 0; i < mat.Textures.size(); i++)
            {
                SortTextures(newMat, mat, folder, i);
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
                    root->MaterialIndex = i;
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
        // TODO: MaterialIndex
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
                    newNode->MaterialIndex = i;
                }
            }
        }
        return newNode;
    }

    void ObjImporter::CreateNewVertex(Mesh &mesh, const VertexIndices& indices) const
    {
        mesh.Vertices.push_back(m_OBJModel->Vertices.at(indices.VertexIndex));

        if (indices.TextureCoordinateIndex != INVALID_ID)
            mesh.TexCoords[OBJ_TEXTURE_COORDINATE_ARRAY_INDEX].push_back(m_OBJModel->TexCoords.at(indices.TextureCoordinateIndex));
        if (indices.NormalIndex != INVALID_ID)
            mesh.Normals.push_back(m_OBJModel->Normals.at(indices.NormalIndex));

        mesh.Indices.push_back(mesh.Vertices.size() - 1);
    }

    void ObjImporter::SortTextures(Material &newMat, const OBJ::Material &mat, const Path& folder, uint32_t i)
    {
        OBJ::TextureType type = (OBJ::TextureType) i;
        const std::string& texturePath = mat.Textures.at(type);
        if (texturePath.empty())
            return;
        bool clampValue = mat.TextureClamps.at(type);

        ImageSettings settings = {};
        settings.Clamp = clampValue ? ClampOption::ClampToBorder : ClampOption::ClampRepeat;

        switch (type)
        {
            case OBJ::Ambient:
                newMat.AlbedoTexture = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::Diffuse:
                newMat.DiffuseTexture = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::Specular:
                newMat.SpecularTexture = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::Emissive:
                newMat.EmissiveTexture = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::Transparency:
                newMat.TransparencyTexture = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::Bump:
                newMat.Bump = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::Shininess:
                newMat.ShininessTexture = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::Roughness:
                newMat.RoughnessTexture = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::Metallic:
                newMat.MetallicTexture = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::Sheen:
                newMat.SheenTexture = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::Clearcoat:
                newMat.ClearcoatTexture = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::ClearcoatRoughness:
                newMat.ClearcoatRoughnessTexture = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::Occlusion:
                newMat.AmbientOcclusionTexture = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::Normal:
                newMat.NormalMap = std::make_unique<Image>(folder / texturePath, settings);
                break;
            case OBJ::ReflectionTop:
            case OBJ::ReflectionBottom:
            case OBJ::ReflectionBack:
            case OBJ::ReflectionFront:
            case OBJ::ReflectionLeft:
            case OBJ::ReflectionRight:
            case OBJ::ReflectionSphere:
                settings.Orientation = (TextureOrientation) (type - 13);
                newMat.ReflectionMaps.push_back(std::make_unique<Image>(folder / texturePath, settings));
                break;
            default:
                break;
        }
    }
}
