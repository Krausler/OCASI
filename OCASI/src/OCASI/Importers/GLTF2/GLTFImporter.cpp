#include "GLTFImporter.h"

#include "OCASI/Core/StringUtil.h"
#include "OCASI/Importers/GLTF2/Json.h"

#include "OCBase/IO/IO.h"

#include "glm/gtc/quaternion.hpp"

using namespace simdjson;

namespace OCASI {

    const size_t BINARY_HEADER_BYTE_SIZE = 12;
    const uint32_t BINARY_HEADER_MAGIC_VALUE = 0x46546C67; // glTF in ASCII
    const uint32_t BINARY_HEADER_VERSION = 2;

    const uint32_t CHUNK_TYPE_JSON = 0x4E4F534A;
    const uint32_t CHUNK_TYPE_BINARY = 0x004E4942;

    bool GLTFImporter::CanLoad(OCBase::FileStreamReader& reader)
    {
        m_FileReader = &reader;
        OCBase::File& f = m_FileReader->GetFile();
        
        if (m_FileReader->GetFile().GetPath().extension() == ".glb")
        {
            auto val = OCBase::IO::Reopen(f, f.GetMode() | OCBase::FileMode::Bin);
            if(!val)
            {
                OCASI_LOG_ERROR("GLTF: {}", val.error().GetErrorMessage());
                return false;
            }

            if (CheckBinaryHeader())
                return true;
        }
        else
        {
            m_Json = new GLTF::Json;
            if(padded_string::load(f.GetPath().string()).get(m_Json->PaddedJsonString))
                return false;
            
            if (!m_Json->Parser.iterate(m_Json->PaddedJsonString).get(m_Json->Json))
                return true;
        }
        return false;
    }

    ExpectedImportT<SharedPtr<Scene>> GLTFImporter::Load3DFile(OCBase::FileStreamReader& reader)
    {
        m_FileReader = &reader;
        
        if (m_FileReader->GetFile().HasFileMode(OCBase::FileMode::Bin))
        {
            auto eGLB = LoadBinary();
            if (!eGLB)
                return UnexpectedF(eGLB.error());
        }
        else
        {
            GLTF::JsonParser parser(*m_FileReader, m_Json);
            auto eAsset = parser.ParseGLTFTextFile()
                    .transform([this](const auto& asset) { m_Asset = asset; });
            if (!eAsset)
                return UnexpectedF(eAsset.error());
        }

        auto result = ConvertToOCASIScene();
        if (!result)
            return UnexpectedF(result.error());

        return m_Scene;
    }

    ExpectedImport GLTFImporter::LoadBinary()
    {
        // Skip the header, as it has already been checked to be valid in the CheckBinaryHeader function
        m_FileReader->SetOffset(BINARY_HEADER_BYTE_SIZE);

        GLBChunk jsonChunk = LoadChunk();
        if (jsonChunk.Type != CHUNK_TYPE_JSON)
            return UnexpectedF(ImportError(ImportError::Type::InvalidParameter, "The first binary chunk must be of type json."));
            
        m_Json = new GLTF::Json;
        m_Json->PaddedJsonString = padded_string((char*)jsonChunk.Data, jsonChunk.ChunkLength);
        
        if (auto error = m_Json->Parser.iterate(m_Json->PaddedJsonString).get(m_Json->Json); error != error_code::SUCCESS)
            return UnexpectedF(ImportError(ImportError::Type::ReadMalfunction, FORMAT("Failed to read json file successfully: {}", simdjson::error_message(error))));

        GLTF::JsonParser parser(*m_FileReader, m_Json);
        auto eAsset = parser.ParseGLTFTextFile()
                .transform([this](const auto& asset) { m_Asset = asset; });
        if (!eAsset)
            return UnexpectedF(eAsset.error());

        // Checking whether there is a second chunk

        // 2 * 4 bytes for chunk info + minimum buffer size (data needs to be aligned by 4)
        constexpr size_t byteSizeToAdd = sizeof(uint32_t) * 2 + 4;
        if (m_FileReader->GetOffset() + byteSizeToAdd < m_FileReader->GetFile().GetSize())
        {
            GLBChunk bufferChunk = LoadChunk();
            if (bufferChunk.Type != CHUNK_TYPE_BINARY)
                return UnexpectedF(ImportError(ImportError::Type::InvalidParameter, "The second binary chunk must be of type binary/data."));

            for (GLTF::Buffer& buffer : m_Asset->Buffers)
            {
                if (!buffer.m_Data)
                {
                    buffer.SetData(bufferChunk.Data);
                    break;
                }
                else
                {
                    return UnexpectedF(ImportError(ImportError::Type::InvalidParameter, "The structure of the GLTF 2.0 GLB file format is strictly defined to ONLY have ONE GLB binary chunk, thus only ONE Json buffer with no data assigned to it is allowed."));
                }
            }
        }

        delete jsonChunk.Data;

        return {};
    }

    GLBChunk GLTFImporter::LoadChunk()
    {
        GLBChunk chunk = {};
        chunk.ChunkLength = m_FileReader->Read<uint32_t>();
        chunk.Type = m_FileReader->Read<uint32_t>();
        chunk.Data = m_FileReader->Read(chunk.ChunkLength);

        // Remove trailing zeros
        for (uint32_t i = chunk.ChunkLength - 1; i > 0; i--)
        {
            if (chunk.Data[i] == '\0')
                chunk.ChunkLength--;
            else
                break;
        }

        return chunk;
    }

    bool GLTFImporter::CheckBinaryHeader()
    {
        uint8_t* data = m_FileReader->Read(BINARY_HEADER_BYTE_SIZE);
        m_FileReader->SetOffset(0);
        GLBHeader* header = (GLBHeader*) data;

        if (!(header->Magic == BINARY_HEADER_MAGIC_VALUE && header->Version == BINARY_HEADER_VERSION && header->FileLength == m_FileReader->GetFile().GetSize()))
            return false;
        return true;
    }
    
    ExpectedImport GLTFImporter::ConvertToOCASIScene()
    {
        m_Scene = MakeShared<Scene>();

        auto& gltfAsset = *m_Asset;
        
        for (auto& gltfScene: gltfAsset.Scenes)
        {
            CreateNodes(gltfScene.GetIndex());
        }

        for (auto& gltfMesh: gltfAsset.Meshes)
        {
            auto e = CreateMesh(gltfMesh.GetIndex());
            if(!e)
                return e;
        }

        for (auto& gltfMaterial : gltfAsset.Materials)
        {
            auto e = CreateMaterial(gltfMaterial.GetIndex());
            if(!e)
                return e;
        }
        
        return {};
    }
    
    void GLTFImporter::CreateNodes(size_t sceneIndex)
    {
        auto& gltfAsset = *m_Asset;
        auto& ocasiScene = *m_Scene;
        auto& gltfScene = gltfAsset.Scenes.at(sceneIndex);

        // When there are multiple scenes, each scene has a root node
        SharedPtr<Node> ocasiRootNode = nullptr;
        if (m_Asset->Scenes.size() > 1)
            ocasiRootNode = m_Scene->RootNodes.emplace_back();

        for (size_t& gltfRootNodeIndex : gltfScene.RootNodes)
        {
            GLTF::Node& gltfRootNode = gltfAsset.Nodes.at(gltfRootNodeIndex);
            auto ocasiNode = MakeShared<Node>();
            
            if (ocasiRootNode)
            {
                ocasiNode->Parent = ocasiRootNode;
                ocasiRootNode->Children.push_back(ocasiNode);
            }
            else
                ocasiScene.RootNodes.push_back(ocasiNode);
            
            if (gltfRootNode.Mesh != INVALID_ID)
                ocasiNode->ModelIndex = gltfRootNode.Mesh;
            
            glm::mat4 translation = glm::translate(translation, gltfRootNode.TrsComponent.Translation);
            glm::mat4 rotation = glm::mat4_cast(gltfRootNode.TrsComponent.Rotation);
            glm::mat4 scale = glm::translate(translation, gltfRootNode.TrsComponent.Scale);
            ocasiNode->LocalTransform = gltfRootNode.LocalTranslationMatrix * translation * rotation * scale;

            TraverseNodes(gltfRootNode, ocasiNode);
        }
    }
    
    void GLTFImporter::TraverseNodes(GLTF::Node& gltfNode, SharedPtr<Node> ocasiNode)
    {
        for (size_t child : gltfNode.Children)
        {
            auto& childGltfNode = m_Asset->Nodes.at(child);

            SharedPtr<Node> childOcasiNode = MakeShared<Node>();
            childOcasiNode->Parent = ocasiNode;
            ocasiNode->Children.push_back(childOcasiNode);
            
            childOcasiNode->ModelIndex = gltfNode.Mesh;
            
            glm::mat4 translation = glm::translate(translation, childGltfNode.TrsComponent.Translation);
            glm::mat4 rotation = glm::mat4_cast(childGltfNode.TrsComponent.Rotation);
            glm::mat4 scale = glm::translate(translation, childGltfNode.TrsComponent.Scale);
            childOcasiNode->LocalTransform = childGltfNode.LocalTranslationMatrix * translation * rotation * scale;

            TraverseNodes(childGltfNode, ocasiNode);
        }
    }
    
    ExpectedImport GLTFImporter::CreateMesh(size_t meshIndex)
    {
        auto& gltfAsset = *m_Asset;
        auto& ocasiScene = *m_Scene;

        OCASI_ASSERT(meshIndex < gltfAsset.Meshes.size());
        auto& gltfMesh = gltfAsset.Meshes.at(meshIndex);
        OCASI_ASSERT(meshIndex == ocasiScene.Models.size());
        auto& ocasiModel = ocasiScene.Models.emplace_back();
        ocasiModel.Meshes.reserve(gltfMesh.Primitives.size());

        for (auto& gltfPrimitive : gltfMesh.Primitives)
        {
            auto& ocasiMesh = ocasiModel.Meshes.emplace_back();
            ocasiMesh.MaterialIndex = gltfPrimitive.MaterialIndex;
            ocasiMesh.FaceMode = ConvertPrimitiveTypeToFaceType(gltfPrimitive.Type);

            if (gltfPrimitive.Indices != INVALID_ID)
            {
                Vector<uint8_t> data;
                auto eData = GetAccessorData(gltfPrimitive.Indices)
                        .transform([&data](const Vector<uint8_t>& val) { data = val; });
                
                if (!eData)
                    return UnexpectedF(eData.error());
                    
                GLTF::ComponentType cType = gltfAsset.Accessors.at(gltfPrimitive.Indices).CompType;
                size_t indicesDataTypeSize = GLTF::ComponentTypeToBytes(cType);
                
                if(!(cType == GLTF::ComponentType::UnsignedInt | cType == GLTF::ComponentType::UnsignedShort))
                    return UnexpectedF(ImportError(ImportError::Type::InvalidParameter, FORMAT("When indices is defined, the referenced accessor must have a componentType of UnsignedInt. Primitive = [{}]", gltfPrimitive.GetIndex())));
                
                ocasiMesh.Indices.resize(data.size() / indicesDataTypeSize);
                
                if(cType != GLTF::ComponentType::UnsignedInt)
                {
                    for (size_t i = 0; i < data.size() / indicesDataTypeSize; i++)
                    {
                        memcpy(&ocasiMesh.Indices[i], &data[i * indicesDataTypeSize], indicesDataTypeSize);
                    }
                }
                else
                {
                    memcpy(ocasiMesh.Indices.data(), data.data(), sizeof(uint32_t) * ocasiMesh.Indices.size());
                }
            }

            for (auto& [attributeName, accessor] : gltfPrimitive.Attributes)
            {
                Vector<uint8_t> data;
                auto eData = GetAccessorData(accessor)
                        .transform([&data](const Vector<uint8_t>& val) { data = val; });
                
                if (!eData)
                    return UnexpectedF(eData.error());
                
                // TODO: Currently the data types are fixed, make these dynamic or something
                if (attributeName == "POSITION")
                {
                    OCASI_ASSERT(data.size() % sizeof(glm::vec3) == 0);

                    ocasiMesh.Vertices.resize(data.size() / sizeof(glm::vec3));
                    memcpy(ocasiMesh.Vertices.data(), data.data(), data.size());
                }
                else if (attributeName == "NORMAL")
                {
                    OCASI_ASSERT(data.size() % sizeof(glm::vec3) == 0);

                    ocasiMesh.Normals.resize(data.size() / sizeof(glm::vec3));
                    memcpy(ocasiMesh.Normals.data(), data.data(), data.size());
                }
                else if (attributeName == "TANGENT")
                {
                     OCASI_ASSERT(data.size() % sizeof(glm::vec4) == 0);

                    ocasiMesh.Tangents.resize(data.size() / sizeof(glm::vec4));
                    memcpy(ocasiMesh.Tangents.data(), data.data(), data.size());
                }
                else if (Util::StartsWith(attributeName, "TEXCOORD_"))
                {
                    const size_t TEX_COORD_STRING = 9;
                    size_t texCoordIndex = std::atoi(&attributeName.at(TEX_COORD_STRING));

                    OCASI_ASSERT(texCoordIndex < ocasiMesh.TexCoords.size());
                    auto& texCoords = ocasiMesh.TexCoords.at(texCoordIndex);

                    OCASI_ASSERT(data.size() % sizeof(glm::vec2) == 0);

                    texCoords.resize(data.size() / sizeof(glm::vec2));
                    memcpy(texCoords.data(), data.data(), data.size());
                }
                else if (Util::StartsWith(attributeName, "COLOR_"))
                {
                    const size_t COLOR_STRING = 6;
                    size_t colorIndex = std::atoi(&attributeName.at(COLOR_STRING));
                    OCASI_ASSERT(data.size() % sizeof(glm::vec4) == 0);

                    ocasiMesh.VertexColours.resize(data.size() / sizeof(glm::vec4));
                    memcpy(ocasiMesh.VertexColours.data(), data.data(), data.size());
                }
            }
        }
        return {};
    }

    ExpectedImport GLTFImporter::CreateMaterial(size_t materialIndex)
    {
        auto& gltfAsset = *m_Asset;
        auto& ocasiScene = *m_Scene;

        OCASI_ASSERT(materialIndex < gltfAsset.Materials.size());
        auto& gltfMaterial = gltfAsset.Materials.at(materialIndex);
        OCASI_ASSERT(materialIndex == ocasiScene.Materials.size());
        auto& ocasiMaterial = ocasiScene.Materials.emplace_back();

        ocasiMaterial.SetName(gltfMaterial.Name);

        // Non-extension material parameters
        {
            if (gltfMaterial.MetallicRoughness)
            {

                ocasiMaterial.SetValue(MATERIAL_ALBEDO_COLOUR, gltfMaterial.MetallicRoughness->BaseColour);
                
                auto eBaseColour = CreateTexture(gltfMaterial.MetallicRoughness->BaseColourTexture)
                        .transform([&ocasiMaterial](const auto& image)
                                   { ocasiMaterial.SetTexture(MATERIAL_TEXTURE_ALBEDO, image); });
                if (!eBaseColour)
                    return UnexpectedF(eBaseColour.error());
                

                ocasiMaterial.SetValue(MATERIAL_USE_COMBINED_METALLIC_ROUGHNESS_TEXTURE, true);
                ocasiMaterial.SetValue(MATERIAL_METALLIC, gltfMaterial.MetallicRoughness->Metallic);
                ocasiMaterial.SetValue(MATERIAL_ROUGHNESS, gltfMaterial.MetallicRoughness->Roughness);
                auto eMetallicRoughness = CreateTexture(gltfMaterial.MetallicRoughness->MetallicRoughnessTexture)
                        .transform([&ocasiMaterial](const auto& image)
                                   { ocasiMaterial.SetTexture(MATERIAL_TEXTURE_METALLIC, image); });
                if (!eMetallicRoughness)
                    return UnexpectedF(eMetallicRoughness.error());
            }

            // Normal texture
            auto eNormal = CreateTexture(gltfMaterial.NormalTexture)
                    .transform([&ocasiMaterial](const auto& image)
                               { ocasiMaterial.SetTexture(MATERIAL_TEXTURE_NORMAL, image); });
            if (!eNormal)
                return UnexpectedF(eNormal.error());
            
            // Occlusion texture
            auto eOcclusion = CreateTexture(gltfMaterial.OcclusionTexture)
                    .transform([&ocasiMaterial](const auto& image)
                               { ocasiMaterial.SetTexture(MATERIAL_TEXTURE_OCCLUSION, image); });
            if (!eOcclusion)
                return UnexpectedF(eOcclusion.error());

            ocasiMaterial.SetValue(MATERIAL_EMISSIVE_COLOUR, gltfMaterial.EmissiveColour);
            auto eEmissive = CreateTexture(gltfMaterial.EmissiveTexture)
                    .transform([&ocasiMaterial](const auto& image)
                               { ocasiMaterial.SetTexture(MATERIAL_EMISSIVE_COLOUR, image); });
            if (!eEmissive)
                return UnexpectedF(eEmissive.error());
            
            
            // TODO: AlphaCutoff, AMode, DoubleSided
        }

        // Extension material parameters
        {
            if (gltfMaterial.ExtEmissiveStrength)
                ocasiMaterial.SetValue(MATERIAL_EMISSIVE_STRENGTH, gltfMaterial.ExtEmissiveStrength->EmissiveStrength);

            if (gltfMaterial.ExtSpecular)
            {
                auto& extSpecular = gltfMaterial.ExtSpecular.value();
                ocasiMaterial.SetValue(MATERIAL_SPECULAR_COLOUR, extSpecular.SpecularColourFactor);
                auto eSpec = CreateTexture(extSpecular.SpecularColourTexture)
                        .transform([&ocasiMaterial](const auto& image)
                        { ocasiMaterial.SetTexture(MATERIAL_TEXTURE_SPECULAR, image); });
                if (!eSpec)
                    return UnexpectedF(eSpec.error());
                
                

                ocasiMaterial.SetValue(MATERIAL_SPECULAR_STRENGTH, extSpecular.SpecularFactor);
                auto eSpecStrength = CreateTexture(extSpecular.SpecularTexture)
                        .transform([&ocasiMaterial](const auto& image)
                                   { ocasiMaterial.SetTexture(MATERIAL_TEXTURE_SPECULAR_STRENGTH, image); });
                if (!eSpecStrength)
                    return UnexpectedF(eSpecStrength.error());
            }

            if (gltfMaterial.ExtIOR)
                ocasiMaterial.SetValue(MATERIAL_IOR, gltfMaterial.ExtIOR->IOR);

            if (gltfMaterial.ExtSpecularGlossiness)
            {
                auto& extPbrSpecular = gltfMaterial.ExtSpecularGlossiness.value();
                ocasiMaterial.SetValue(MATERIAL_ALBEDO_COLOUR, extPbrSpecular.DiffuseFactor);
                auto eDiffuse = CreateTexture(extPbrSpecular.DiffuseTexture)
                        .transform([&ocasiMaterial](const auto& image)
                                   { ocasiMaterial.SetTexture(MATERIAL_TEXTURE_ALBEDO, image); });
                if(!eDiffuse)
                    return UnexpectedF(eDiffuse.error());
                

                ocasiMaterial.SetValue(MATERIAL_SPECULAR_COLOUR, extPbrSpecular.SpecularFactor);
                auto eSpecGlossiness = CreateTexture(extPbrSpecular.SpecularGlossinessTexture)
                        .transform([&ocasiMaterial](const auto& image)
                                   { ocasiMaterial.SetTexture(MATERIAL_TEXTURE_SPECULAR, image); });
                if(!eSpecGlossiness)
                    return UnexpectedF(eSpecGlossiness.error());
                

                ocasiMaterial.SetValue(MATERIAL_SPECULAR_STRENGTH, extPbrSpecular.GlossinessFactor);
            }

            if (gltfMaterial.ExtAnisotropy)
            {
                auto& extAnisotropy = gltfMaterial.ExtAnisotropy.value();
                ocasiMaterial.SetValue(MATERIAL_ANISOTROPY, extAnisotropy.AnisotropyFactor);
                ocasiMaterial.SetValue(MATERIAL_ANISOTROPY_ROTATION, extAnisotropy.AnisotropyDirection);

                ocasiMaterial.SetValue(MATERIAL_USE_COMBINED_ANISOTROPY_ANISOTROPY_ROTATION_TEXTURE, true);
                auto eAnisotropy = CreateTexture(extAnisotropy.AnisotropyTexture)
                        .transform([&ocasiMaterial](const auto& image)
                                   { ocasiMaterial.SetTexture(MATERIAL_TEXTURE_ANISOTROPY, image); });
                if(!eAnisotropy)
                    return UnexpectedF(eAnisotropy.error());
            }

            if (gltfMaterial.ExtClearcoat)
            {
                auto& extClearcoat = gltfMaterial.ExtClearcoat.value();
                ocasiMaterial.SetValue(MATERIAL_CLEARCOAT, extClearcoat.ClearcoatFactor);
                auto eClearCoat = CreateTexture(extClearcoat.ClearcoatTexture)
                        .transform([&ocasiMaterial](const auto& image)
                                   { ocasiMaterial.SetTexture(MATERIAL_TEXTURE_CLEARCOAT, image); });
                if(!eClearCoat)
                    return UnexpectedF(eClearCoat.error());
                

                ocasiMaterial.SetValue(MATERIAL_CLEARCOAT_ROUGHNESS, extClearcoat.ClearcoatRoughnessFactor);
                auto eClearCoatRoughness = CreateTexture(extClearcoat.ClearcoatRoughnessTexture)
                        .transform([&ocasiMaterial](const auto& image)
                                   { ocasiMaterial.SetTexture(MATERIAL_TEXTURE_CLEARCOAT_ROUGHNESS, image); });
                if(!eClearCoatRoughness)
                    return UnexpectedF(eClearCoatRoughness.error());

                
                auto eClearCoatNormal = CreateTexture(extClearcoat.ClearcoatNormalTexture)
                        .transform([&ocasiMaterial](const auto& image)
                                   { ocasiMaterial.SetTexture(MATERIAL_TEXTURE_CLEARCOAT_NORMAL, image); });
                if(!eClearCoatNormal)
                    return UnexpectedF(eClearCoatNormal.error());
            }

            // TODO: Iridescence, Volume, Sheen, Transmission

        }
        
        return {};
    }

    ExpectedImportT<SharedPtr<Image>> GLTFImporter::CreateTexture(std::optional<GLTF::TextureInfo>& texInfo)
    {
        if (!texInfo.has_value())
            return nullptr;

        auto& gltfAsset = *m_Asset;

        const GLTF::TextureInfo& gltfInfo = texInfo.value();

        OCASI_ASSERT(gltfInfo.Texture < gltfAsset.Textures.size());
        GLTF::Texture& gltfTexture = gltfAsset.Textures.at(gltfInfo.Texture);

        OCASI_ASSERT(gltfTexture.Source != INVALID_ID, "Do not know what to do with a texture that does not contain an image source. Texture json index: {}", texInfo->Texture);
        OCASI_ASSERT(gltfTexture.Source < gltfAsset.Images.size());
        GLTF::Image& gltfImage = gltfAsset.Images.at(gltfTexture.Source);

        ImageSettings settings = {};
        if (gltfTexture.Sampler != INVALID_ID)
        {
            OCASI_ASSERT(gltfTexture.Sampler < gltfAsset.Samplers.size());
            GLTF::Sampler& gltfSampler = gltfAsset.Samplers.at(gltfTexture.Sampler);

            // For implementation and ease of use, the clamp option
            // will be using the UVWrapT value.
            settings.Clamp = gltfSampler.WrapT == GLTF::UVWrap::Repeat ? ClampOption::Repeat : (gltfSampler.WrapT == GLTF::UVWrap::ClampToEdge ? ClampOption::ClampToEdge : ClampOption::MirroredRepeat);

            settings.MagFilter = ConvertMinMagFilterToFilterOption(gltfSampler.MagFilter);
            settings.MinFilter = ConvertMinMagFilterToFilterOption(gltfSampler.MinFilter);
        }
        
        if (gltfImage.BufferView != INVALID_ID)
        {
            size_t unused = 0;
            Vector<uint8_t> data;
            auto eData = GetBufferViewData(gltfImage.BufferView, 0, unused)
                    .transform([&data](const std::span<uint8_t>& val)
                    {
                        data.resize(val.size());
                        std::memcpy(data.data(), val.data(), val.size());
                    });
            
            if (!eData)
                return UnexpectedF(eData.error());

            // TODO: Maybe remove this as it is not used
            // When bufferView is defined, mimeType must also be defined
            OCASI_ASSERT(!gltfImage.MimeType.empty());
            ImageType type = ConvertMimeTypeToImagType(gltfImage.MimeType);

            return MakeShared<Image>(std::move(data), settings);
        }
        else if (!gltfImage.URI.empty())
        {
            String uri = Util::URIUnescapedString(gltfImage.URI);
            if (Util::StartsWith(uri, "data:"))
            {
                String data = gltfImage.URI.substr(uri.find(':'));
                size_t readSize = 0;
                uint8_t* binaryData = Util::DecodeBase64(data, readSize);
                if (!binaryData)
                    return UnexpectedF(ImportError(ImportError::Type::RequirementsNotMet, FORMAT("Failed to decode Base64 encoded string. Image = [{}]", gltfImage.GetIndex())));
                
                Vector<uint8_t> binaryDataVector(readSize);

                memcpy(binaryDataVector.data(), binaryData, readSize);

                return MakeShared<Image>(std::move(binaryDataVector), settings);
            }
            else if (Path path = m_FileReader->GetFile().GetPath().parent_path() / uri; std::filesystem::exists(path))
            {
                return std::make_unique<Image>(path, settings);
            }
            else
            {
                return UnexpectedF(ImportError(ImportError::Type::InvalidParameter, FORMAT("When an image has a defined URI component, it must either be a valid Base64 encoded string or"
                                                                                    "a valid relative path to an image. Image = [{}]", gltfImage.GetIndex())));
            }
        }
        else
        {
            return UnexpectedF(ImportError(ImportError::Type::MissingParameter, FORMAT("Image must either have an URI component or a reference to a bufferView. Image = [{}]", gltfImage.GetIndex())));
        }
    }
    
    ExpectedImportT<std::span<uint8_t>> GLTFImporter::GetBufferViewData(size_t bufferViewIndex, size_t accessorOffset, size_t& outByteStride)
    {
        auto& asset = *m_Asset;
        OCASI_ASSERT(bufferViewIndex < asset.BufferViews.size());
        auto& bufferView = asset.BufferViews.at(bufferViewIndex);
        outByteStride = bufferView.ByteStride;

        OCASI_ASSERT(bufferView.Buffer < asset.Buffers.size());
        auto& buffer = asset.Buffers.at(bufferView.Buffer);
        
        return buffer.Get(bufferView.ByteLength, bufferView.ByteOffset + accessorOffset);
    }
    
    ExpectedImportT<Vector<uint8_t>> GLTFImporter::GetAccessorData(size_t accessorIndex)
    {
        auto& asset = *m_Asset;

        OCASI_ASSERT(accessorIndex < asset.Accessors.size());
        auto& accessor = asset.Accessors.at(accessorIndex);
        
        // TODO: Figure out what to do in this case.
        if (accessor.BufferView == INVALID_ID)
            return UnexpectedF(ImportError(ImportError::Type::InvalidParameter, "Accessor {} does not reference a valid bufferView, which is not supported."));

        // This is the accessor offset, not the buffer view offset
        size_t elementSize = GLTF::ComponentTypeToBytes(accessor.CompType) * (size_t) accessor.Type;

        size_t byteStride = 0;
        auto eData = GetBufferViewData(accessor.BufferView, accessor.ByteOffset, byteStride).
                and_then([&byteStride, &elementSize, &accessor](std::span<uint8_t> data)
                {
                    // The byteStride specifies the number of bytes for each element. This includes both element size and padding.
                    Vector<uint8_t> actualData(data.size());
                    std::memcpy(actualData.data(), data.data(), data.size());
                    if (byteStride != 0 && byteStride != elementSize)
                    {
                        for (size_t i = 0; i < accessor.ElementCount; i++)
                        {
                            memcpy(&data[i * elementSize], &data[i * byteStride], elementSize);
                        }
                        // Now the data tightly packed
                        actualData.resize(elementSize * accessor.ElementCount);
                    }
                    
                    return ExpectedImportT<Vector<uint8_t>>(actualData);
                });

        if (!eData)
            return UnexpectedF(eData.error());
        
        Vector<uint8_t> data = eData.value();

        if (accessor.SparseAccessor.has_value())
        {
            auto& sparse = accessor.SparseAccessor.value();
            OCASI_ASSERT(sparse.Indices.BufferView < asset.BufferViews.size());

            size_t sparseIndicesByteStride; // ignored
            std::span<uint8_t> sparseIndicesData;
            auto eIndices = GetBufferViewData(sparse.Indices.BufferView, sparse.Indices.ByteOffset, sparseIndicesByteStride)
                    .transform([&sparseIndicesData](const std::span<uint8_t>& indices) { sparseIndicesData = indices; });
            
            if (!eIndices)
                return UnexpectedF(eIndices.error());
                
            size_t sparseValuesByteStride; // ignored
            std::span<uint8_t> sparseValuesData;
            auto eValues = GetBufferViewData(sparse.Values.BufferView, sparse.Values.ByteOffset, sparseValuesByteStride)
                    .transform([&sparseValuesData](const std::span<uint8_t>& values) { sparseValuesData = values; });
            
            if (!eValues)
                return UnexpectedF(eValues.error());

            // TODO: Create a new vector, copying over the indices
            // HACK: For indexing a conversion from bytes to a number is needed
            for (size_t i = 0; i < sparse.ElementCount; i++)
            {
                uint32_t index = 0;
                switch(sparse.Indices.CompType)
                {
                    case GLTF::ComponentType::Short:
                    case GLTF::ComponentType::UnsignedShort:
                    {
                        memcpy(&index, &sparseIndicesData[i * sizeof(uint16_t)], sizeof(uint16_t));
                        break;
                    }
                    case GLTF::ComponentType::UnsignedInt:
                    {
                        memcpy(&index, &sparseIndicesData[i * sizeof(uint32_t)], sizeof(uint32_t));
                        break;
                    }
                    default:
                        return UnexpectedF(ImportError(ImportError::Type::InvalidParameter, FORMAT("Unsupported component type {} used for sparse accessor. Accessor = [{}]", (int) sparse.Indices.CompType, accessorIndex)));
                }

                memcpy(&data[index * elementSize], &sparseValuesData[i * elementSize], elementSize);
            }
        }

        return data;
    }

    // TODO: Use the already existing FilterOption struct for the GLTF implementation. This is completely unnecessary!
    FilterOption GLTFImporter::ConvertMinMagFilterToFilterOption(GLTF::MinMagFilter filter)
    {
        switch (filter)
        {
            case GLTF::MinMagFilter::Nearest:
                return FilterOption::Nearest;
            case GLTF::MinMagFilter::Linear:
                return FilterOption::Linear;
            case GLTF::MinMagFilter::NearestMipMapNearest:
                return FilterOption::NearestMipMapNearest;
            case GLTF::MinMagFilter::NearestMipMapLinear:
                return FilterOption::NearestMipMapLinear;
            case GLTF::MinMagFilter::LinearMipMapNearest:
                return FilterOption::LinearMipMapNearest;
            case GLTF::MinMagFilter::LinearMipMapLinear:
                return FilterOption::LinearMipMapLinear;
            default:
                return FilterOption::Linear;
        }
    }
    
    FaceType GLTFImporter::ConvertPrimitiveTypeToFaceType(GLTF::PrimitiveType primitive)
    {
        switch (primitive)
        {
            case GLTF::PrimitiveType::Point:
                return FaceType::Point;
            case GLTF::PrimitiveType::Line:
                return FaceType::Line;
            case GLTF::PrimitiveType::Triangle:
                return FaceType::Triangle;
            default:
                return FaceType::None;
        }
    }
    
    ImageType GLTFImporter::ConvertMimeTypeToImagType(const String& mimeType)
    {
        if (mimeType == "image/png")
            return ImageType::PNG;
        else if (mimeType == "image/jpeg")
            return ImageType::JPEG;
        else
            return ImageType::None;
    }
}