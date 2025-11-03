#include "ObjImporter.h"

#include "OCASI/Importers/OBJ/FileParser.h"
#include "OCASI/Importers/OBJ/MtlParser.h"

#include "OCBase/IO/IO.h"

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
    struct hash<OCASI::VertexIndices>
    {
        std::size_t operator()(const OCASI::VertexIndices& v) const
        {
            return std::hash<size_t>()(v.VertexIndex) + std::hash<size_t>()(v.TextureCoordinateIndex) + std::hash<size_t>()(v.NormalIndex);
        }
    };
}

namespace OCASI {

    // OBJ files only support one set of texture coordinates
    constexpr uint8_t OBJ_TEXTURE_COORDINATE_ARRAY = 0;
    
    bool ObjImporter::CanLoad(OCBase::FileStreamReader& reader)
    {
        m_FileReader = &reader;
        
        // Check the first 100 lines of the OBJ file and search for valid tokens
        const Vector<String> TOKENS = { "v ", "vn ", "vt ", "mtlib ", "f ", "usemtl " };
        const uint32_t CHECKED_LINES = 100;
        
        bool found = false;
        for (uint32_t i = 0; i < CHECKED_LINES; i++)
        {
            String line;
            if (!m_FileReader->NextLine(line))
                break;
            
            if (line.empty())
                continue;
            
            for (const String& token : TOKENS)
            {
                if(line.starts_with(token))
                {
                    found = true;
                    // Easiest way to break out of a nested for loop
                    goto doubleLoopBreak;
                }
            }
        }
        // goto label
        doubleLoopBreak:
        
        m_FileReader->SetOffset(0);
        
        return found;
    }

    ExpectedImportT<SharedPtr<Scene>> ObjImporter::Load3DFile(OCBase::FileStreamReader& reader)
    {
        m_FileReader = &reader;
        
        OBJ::FileParser objParser(*m_FileReader);
        auto e = objParser.ParseOBJFile()
                .transform([this](const auto& model) { m_OBJModel = model; });
        if (!e)
            return UnexpectedF(e.error());

        Path folder = m_FileReader->GetFile().GetPath().parent_path();

        if(!m_OBJModel->MTLFilePath.empty())
        {
            auto eMTLFile = OCBase::IO::Open(folder / m_OBJModel->MTLFilePath, OCBase::FileMode::Read)
                    .transform_error([this](const auto& error)
                    {
                        OCASI_LOG_ERROR("Couldn't load MTL file at relative location {}: {}. Omitting the MTL file.", m_OBJModel->MTLFilePath, error.GetErrorMessage());
                        return error;
                    });
            
            if (!eMTLFile.has_value())
                return UnexpectedF(ImportError(ImportError::Type::ReadMalfunction, FORMAT("An error occurred while loading a MTL file: {}", eMTLFile.error().GetErrorMessage())));
            else
            {
                OCBase::FileStreamReader mtlReader(eMTLFile.value());
                OBJ::MtlParser mtlParser(m_OBJModel, mtlReader);
                auto eMTL = mtlParser.ParseMTLFile();
                mtlReader.GetFile().Close();
                if (!eMTL)
                    return UnexpectedF(eMTL.error());
            }
        }

        return ConvertToOCASIScene(folder);
    }

    SharedPtr<Scene> ObjImporter::ConvertToOCASIScene(const Path& folder)
    {
        m_OutputScene = MakeShared<Scene>();

//        for (glm::vec3& v : m_OBJModel->Vertices)
//            v = { v.x, v.y, -v.z };
//
//        for (glm::vec3& n : m_OBJModel->Normals)
//            n = { n.x, n.y, -n.z };

        /// Material conversion

        for (const auto& [name, mat] : m_OBJModel->Materials)
        {
            Material& newMat = m_OutputScene->Materials.emplace_back();
            newMat.SetName(mat.Name);

            if (mat.PBRExtension.has_value())
            {
                const OBJ::PBRMaterialExtension& pbrExtension = mat.PBRExtension.value();

                newMat.SetValue(MATERIAL_ROUGHNESS, pbrExtension.Roughness);
                newMat.SetValue(MATERIAL_METALLIC, pbrExtension.Metallic);
                newMat.SetValue(MATERIAL_IOR, pbrExtension.IOR);
                newMat.SetValue(MATERIAL_ANISOTROPY, pbrExtension.Anisotropy);
                newMat.SetValue(MATERIAL_ANISOTROPY_ROTATION, pbrExtension.AnisotropyRotation);
                newMat.SetValue(MATERIAL_CLEARCOAT, pbrExtension.Clearcoat);
                newMat.SetValue(MATERIAL_CLEARCOAT_ROUGHNESS, pbrExtension.ClearcoatRoughness);
            }

            newMat.SetValue(MATERIAL_DIFFUSE_COLOUR, mat.DiffuseColour);
            newMat.SetValue(MATERIAL_AMBIENT_COLOUR, mat.AmbientColour);
            newMat.SetValue(MATERIAL_SPECULAR_COLOUR, mat.SpecularColour);
            newMat.SetValue(MATERIAL_SPECULAR_STRENGTH, mat.Shininess);
            newMat.SetValue(MATERIAL_EMISSIVE_COLOUR, mat.EmissiveColour);
            newMat.SetValue(MATERIAL_TRANSPARENCY, mat.Opacity);

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

    SharedPtr<Node> ObjImporter::CreateNodes(const OBJ::Object& o)
    {
        SharedPtr<Node> node = std::make_unique<Node>();
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
            SharedPtr<Node> groupNode = std::make_unique<Node>();
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
        // To generate indices and remove duplicate vertices, we have to keep track of all unique VertexIndices.
        // This means that for every face, the indices into the global vertex arrays (vertex array, normal array, texture
        // coordinate array) have to be checked against all already loaded indices. If there is a match, we just use the
        // index of that matching vertex in the indices array.
        HashMap<VertexIndices, size_t> lookUpTable;
        size_t newIndex = 0;
        
        for (OBJ::Face f : m.Faces)
        {
            for (size_t i = 0; i < (size_t) f.Type; i++)
            {
                VertexIndices indices = { f.VertexIndices.at(i),
                                          !f.TextureCoordinateIndices.empty() ? f.TextureCoordinateIndices.at(i) : INVALID_ID,
                                          !f.NormalIndices.empty() ? f.NormalIndices.at(i) : INVALID_ID };

                auto result = lookUpTable.try_emplace(indices, newIndex);
                if (result.second)
                {
                    CreateNewVertex(outMesh, indices, newIndex++);
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
                if (m_OutputScene->Materials.at(i).GetName() == m.MaterialName)
                    outMesh.MaterialIndex = i;
            }
        }

        outMesh.Dim = m.Dim;
        outMesh.FaceMode = m.FaceType;

        return outMesh;
    }

    void ObjImporter::CreateNewVertex(Mesh& mesh, const VertexIndices& indices, size_t newIndex) const
    {
        mesh.Vertices.push_back(m_OBJModel->Vertices.at(indices.VertexIndex));
        if (!m_OBJModel->VertexColours.empty())
            mesh.VertexColours.push_back(m_OBJModel->VertexColours.at(indices.VertexIndex));

        if (indices.TextureCoordinateIndex != INVALID_ID)
            mesh.TexCoords[OBJ_TEXTURE_COORDINATE_ARRAY].push_back(m_OBJModel->TexCoords.at(indices.TextureCoordinateIndex));
        if (indices.NormalIndex != INVALID_ID)
            mesh.Normals.push_back(m_OBJModel->Normals.at(indices.NormalIndex));

        mesh.Indices.push_back(newIndex);
    }

    ExpectedImport ObjImporter::SortTextures(Material& newMat, const OBJ::Material &mat, const Path& folder, size_t i)
    {
        // This value is needed to convert the OBJ::TextureType to a OCASI::TextureOrientation
        // for reflection textures. In OBJ, each side of the cube map is provided using a single
        // image. Value 8 is just the mapping value that needs to be subtracted from the TextureType
        // to be able to cast the integer value of the enum to the integer value of the TextureOrientation.
        const uint8_t REFLECTION_TEXTURE_NORMALIZER = 8;

        OBJ::TextureType type = (OBJ::TextureType) i;
        String texturePath = mat.Textures.at(type);

        if (texturePath.empty())
            return {};

        ImageSettings settings = {};
        settings.Clamp = mat.TextureClamps.at(i) ? ClampOption::ClampToEdge : ClampOption::Repeat;

        switch (type)
        {
            case OBJ::TextureType::Roughness:
            {
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_ROUGHNESS, image);
                break;
            }
            case OBJ::TextureType::Metallic:
            {
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_METALLIC, image);
                break;
            }
            case OBJ::TextureType::Sheen:
                // TODO: Not Implemented
                break;
            case OBJ::TextureType::Clearcoat:
            {
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_CLEARCOAT, image);
                break;
            }
            case OBJ::TextureType::ClearcoatRoughness:
            {
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_CLEARCOAT_ROUGHNESS, image);
                break;
            }
            case OBJ::TextureType::Occlusion:
            {
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_OCCLUSION, image);
                break;
            }
            case OBJ::TextureType::Diffuse:
            {
                // Diffuse textures are classified as the object base color
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_ALBEDO, image);
                break;
            }
            case OBJ::TextureType::Ambient:
            {
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_AMBIENT, image);
                break;
            }
            case OBJ::TextureType::Specular:
            {
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_SPECULAR, image);
                break;
            }
            case OBJ::TextureType::Emissive:
            {
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_EMISSIVE, image);
                break;
            }
            case OBJ::TextureType::Transparency:
            {
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_TRANSPARENCY, image);
                break;
            }
            case OBJ::TextureType::Shininess:
            {
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_SPECULAR_STRENGTH, image);
                break;
            }
            case OBJ::TextureType::Normal:
            {
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_NORMAL, image);
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
                auto image = MakeShared<Image>(folder / texturePath, settings);
                newMat.SetTexture(MATERIAL_TEXTURE_REFLECTION_MAP_TOP + type - REFLECTION_TEXTURE_NORMALIZER, image);
                break;
            }
            default:
                return UnexpectedF(ImportError(ImportError::Type::InvalidParameter, FORMAT("Unknown texture type: {}", (size_t)type)));
        }
    }
}
