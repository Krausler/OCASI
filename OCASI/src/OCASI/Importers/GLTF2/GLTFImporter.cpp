#include "GLTFImporter.h"

#include "OCASI/Core/BinaryReader.h"
#include "OCASI/Core/StringUtil.h"
#include "Json.h"

#include "glm/gtc/quaternion.hpp"

using namespace simdjson;

namespace OCASI {

    const size_t BINARY_HEADER_BYTE_SIZE = 12;
    const uint32_t BINARY_HEADER_MAGIC_VALUE = 0x46546C67;
    const uint32_t BINARY_HEADER_VERSION = 2;

    const uint32_t CHUNK_TYPE_JSON = 0x4E4F534A;
    const uint32_t CHUNK_TYPE_BINARY = 0x004E4942;

    bool GLTFImporter::CanLoad(FileReader& reader)
    {
        m_FileReader = &reader;
        
        if (m_FileReader->GetPath().extension() == ".glb")
        {
            m_FileReader->SetBinary();
            
            if (m_FileReader->GetFileSize() >= std::numeric_limits<uint32_t>::max())
                return false;

            if (CheckBinaryHeader())
                return true;
        }
        else
        {
            m_Json = new GLTF::Json;
            if(padded_string::load(m_FileReader->GetPath().string()).get(m_Json->PaddedJsonString))
                return false;
            
            if (!m_Json->Parser.iterate(m_Json->PaddedJsonString).get(m_Json->Json))
                return true;
        }
        return false;
    }

    SharedPtr<Scene> GLTFImporter::Load3DFile(FileReader& reader)
    {
        m_FileReader = &reader;
        
        if (m_FileReader->IsBinary())
        {
            if (!LoadBinary())
                return nullptr;
        }
        else
        {
            GLTF::JsonParser parser(*m_FileReader, m_Json);
            if (!(m_Asset = parser.ParseGLTFTextFile()))
                return nullptr;
        }

        ConvertToOCASIScene();

        return m_Scene;
    }

    bool GLTFImporter::LoadBinary()
    {
        BinaryReader bReader(*m_FileReader);
        // Skip the header, as it has already been checked to be valid in the CheckBinaryHeader function
        bReader.SetPointer(BINARY_HEADER_BYTE_SIZE);

        GLBChunk jsonChunk = LoadChunk(bReader);
        if (jsonChunk.Type != CHUNK_TYPE_JSON)
            throw FailedImportError("First binary chunk must be of type json.");
            
        m_Json = new GLTF::Json;
        m_Json->PaddedJsonString = padded_string((char*)jsonChunk.Data, jsonChunk.ChunkLength);
        
        if (auto error = m_Json->Parser.iterate(m_Json->PaddedJsonString).get(m_Json->Json); error != error_code::SUCCESS)
            throw FailedImportError(FORMAT("Can't read json file: {}", simdjson::error_message(error)));

        GLTF::JsonParser parser(*m_FileReader, m_Json);
        if (!(m_Asset = parser.ParseGLTFTextFile()))
            return false;

        // Checking whether there is a second chunk

        // 2 * chunk info + minimum buffers size (data needs to be aligned by 4)
        constexpr size_t byteSizeToAdd = sizeof(uint32_t) * 2 + 4;
        if (bReader.GetPointer() + byteSizeToAdd < m_FileReader->GetFileSize())
        {
            GLBChunk bufferChunk = LoadChunk(bReader);
            if (bufferChunk.Type != CHUNK_TYPE_BINARY)
                throw FailedImportError("Second binary chunk must be of type binary.");

            bool found = false;
            for (GLTF::Buffer& buffer : m_Asset->Buffers)
            {
                if (!buffer.m_Data && !found)
                {
                    buffer.SetData(bufferChunk.Data);
                    found = true;
                }
                else
                {
                    throw FailedImportError("GLB file defines more then one binary chunk.");
                }
            }
        }

        delete jsonChunk.Data;

        return true;
    }

    GLBChunk GLTFImporter::LoadChunk(BinaryReader& bReader)
    {
        GLBChunk chunk = {};
        chunk.ChunkLength = bReader.GetUint32();
        chunk.Type = bReader.GetUint32();
        chunk.Data = bReader.Get(chunk.ChunkLength);

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
        uint8_t data[BINARY_HEADER_BYTE_SIZE];
        m_FileReader->GetBytes(data, BINARY_HEADER_BYTE_SIZE);
        m_FileReader->Set0();
        BinaryReader bReader(data, BINARY_HEADER_BYTE_SIZE);
        GLBHeader header = bReader.GetType<GLBHeader>();

        if (!(header.Magic == BINARY_HEADER_MAGIC_VALUE && header.Version == BINARY_HEADER_VERSION && header.FileLength == m_FileReader->GetFileSize()))
            return false;
        return true;
    }

    void GLTFImporter::ConvertToOCASIScene()
    {
        m_Scene = MakeShared<Scene>();

        auto& gltfAsset = *m_Asset;
        
        for (auto& gltfScene: gltfAsset.Scenes)
        {
            CreateNodes(gltfScene.GetIndex());
        }

        for (auto& gltfMesh: gltfAsset.Meshes)
        {
            CreateMesh(gltfMesh.GetIndex());
        }

        for (auto& gltfMaterial : gltfAsset.Materials)
        {
            CreateMaterial(gltfMaterial.GetIndex());
        }

    }

    void GLTFImporter::CreateNodes(size_t sceneIndex)
    {
        auto& gltfAsset = *m_Asset;
        auto& ocasiScene = *m_Scene;
        auto& gltfScene = gltfAsset.Scenes.at(sceneIndex);

        // When there are multiple scenes, each scene has a root node
        SharedPtr<Node> ocasiRootNode = nullptr;
        if (m_Asset->Scenes.size() > 1)
            ocasiRootNode = m_Scene->RootNodes.emplace_back();;

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

    void GLTFImporter::CreateMesh(size_t meshIndex)
    {
        auto& gltfAsset = *m_Asset;
        auto& ocasiScene = *m_Scene;

        OCASI_ASSERT(meshIndex < gltfAsset.Meshes.size());
        auto& gltfMesh = gltfAsset.Meshes.at(meshIndex);
        OCASI_ASSERT(meshIndex == ocasiScene.Models.size());
        auto& ocasiModel = ocasiScene.Models.emplace_back();

        for (auto& gltfPrimitive : gltfMesh.Primitives)
        {
            auto& ocasiMesh = ocasiModel.Meshes.emplace_back();
            ocasiMesh.MaterialIndex = gltfPrimitive.MaterialIndex;
            ocasiMesh.FaceMode = ConvertPrimitiveTypeToFaceType(gltfPrimitive.Type);

            if (gltfPrimitive.Indices != INVALID_ID)
            {
                std::vector<uint8_t> data = GetAccessorData(gltfPrimitive.Indices);
                OCASI_ASSERT(!data.empty());

                size_t indexDataTypeSize = GLTF::ComponentTypeToBytes(gltfAsset.Accessors.at(gltfPrimitive.Indices).CompType);
                OCASI_ASSERT(data.size() % indexDataTypeSize == 0)
                
                ocasiMesh.Indices.resize(data.size() / indexDataTypeSize);
                for (size_t i = 0; i < data.size(); i += indexDataTypeSize)
                    std::memcpy(&ocasiMesh.Indices[(i == 0 ? 0 : i / indexDataTypeSize)], &data[i], indexDataTypeSize);
            }

            for (auto& [attributeName, accessor] : gltfPrimitive.Attributes)
            {
                // TODO: Currently the data types are fixed, make these dynamic or something
                if (attributeName == "POSITION")
                {
                    std::vector<uint8_t> data = GetAccessorData(accessor);
                    OCASI_ASSERT(!data.empty() && data.size() % sizeof(glm::vec3) == 0);

                    ocasiMesh.Vertices.resize(data.size() / sizeof(glm::vec3));
                    std::memcpy(ocasiMesh.Vertices.data(), data.data(), data.size());
                }
                else if (attributeName == "NORMAL")
                {
                    std::vector<uint8_t> data = GetAccessorData(accessor);
                    OCASI_ASSERT(!data.empty() && data.size() % sizeof(glm::vec3) == 0);

                    ocasiMesh.Normals.resize(data.size() / sizeof(glm::vec3));
                    std::memcpy(ocasiMesh.Normals.data(), data.data(), data.size());
                }
                else if (attributeName == "TANGENT")
                {
                     std::vector<uint8_t> data = GetAccessorData(accessor);
                     OCASI_ASSERT(!data.empty() && data.size() % sizeof(glm::vec4) == 0);

                    ocasiMesh.Tangents.resize(data.size() / sizeof(glm::vec4));
                    std::memcpy(ocasiMesh.Tangents.data(), data.data(), data.size());
                }
                else if (Util::StartsWith(attributeName, "TEXCOORD_"))
                {
                    const size_t TEX_COORD_STRING = 9;
                    size_t texCoordIndex = std::atoi(&attributeName.at(TEX_COORD_STRING));

                    OCASI_ASSERT(texCoordIndex < ocasiMesh.TexCoords.size());
                    auto& texCoords = ocasiMesh.TexCoords.at(texCoordIndex);

                    std::vector<uint8_t> data = GetAccessorData(accessor);
                    OCASI_ASSERT(!data.empty() && data.size() % sizeof(glm::vec2) == 0);

                    texCoords.resize(data.size() / sizeof(glm::vec2));
                    std::memcpy(texCoords.data(), data.data(), data.size());
                }
                else if (Util::StartsWith(attributeName, "COLOR_"))
                {
                    const size_t COLOR_STRING = 6;
                    size_t colorIndex = std::atoi(&attributeName.at(COLOR_STRING));

                    std::vector<uint8_t> data = GetAccessorData(accessor);
                    OCASI_ASSERT(!data.empty() && data.size() % sizeof(glm::vec4) == 0);

                    ocasiMesh.VertexColours.resize(data.size() / sizeof(glm::vec4));
                    std::memcpy(ocasiMesh.VertexColours.data(), data.data(), data.size());
                }
            }
        }
    }

    void GLTFImporter::CreateMaterial(size_t materialIndex)
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
                ocasiMaterial.SetTexture(MATERIAL_TEXTURE_ALBEDO, CreateTexture(gltfMaterial.MetallicRoughness->BaseColourTexture));

                ocasiMaterial.SetValue(MATERIAL_USE_COMBINED_METALLIC_ROUGHNESS_TEXTURE, true);
                ocasiMaterial.SetValue(MATERIAL_METALLIC, gltfMaterial.MetallicRoughness->Metallic);
                ocasiMaterial.SetTexture(MATERIAL_TEXTURE_METALLIC, CreateTexture(gltfMaterial.MetallicRoughness->MetallicRoughnessTexture));

                ocasiMaterial.SetValue(MATERIAL_ROUGHNESS, gltfMaterial.MetallicRoughness->Roughness);
            }

            ocasiMaterial.SetTexture(MATERIAL_TEXTURE_NORMAL, CreateTexture(gltfMaterial.NormalTexture));
            ocasiMaterial.SetTexture(MATERIAL_TEXTURE_OCCLUSION, CreateTexture(gltfMaterial.OcclusionTexture));

            ocasiMaterial.SetValue(MATERIAL_EMISSIVE_COLOUR, gltfMaterial.EmissiveColour);
            ocasiMaterial.SetTexture(MATERIAL_TEXTURE_EMISSIVE, CreateTexture(gltfMaterial.EmissiveTexture));

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
                ocasiMaterial.SetTexture(MATERIAL_TEXTURE_SPECULAR, CreateTexture(extSpecular.SpecularColourTexture));

                ocasiMaterial.SetValue(MATERIAL_SPECULAR_STRENGTH, extSpecular.SpecularFactor);
                ocasiMaterial.SetTexture(MATERIAL_TEXTURE_SPECULAR_STRENGTH, CreateTexture(extSpecular.SpecularTexture));
            }

            if (gltfMaterial.ExtIOR)
                ocasiMaterial.SetValue(MATERIAL_IOR, gltfMaterial.ExtIOR->IOR);

            if (gltfMaterial.ExtSpecularGlossiness)
            {
                auto& extPbrSpecular = gltfMaterial.ExtSpecularGlossiness.value();
                ocasiMaterial.SetValue(MATERIAL_ALBEDO_COLOUR, extPbrSpecular.DiffuseFactor);
                ocasiMaterial.SetTexture(MATERIAL_TEXTURE_ALBEDO, CreateTexture(extPbrSpecular.DiffuseTexture));

                ocasiMaterial.SetValue(MATERIAL_SPECULAR_COLOUR, extPbrSpecular.SpecularFactor);
                ocasiMaterial.SetTexture(MATERIAL_TEXTURE_SPECULAR, CreateTexture(extPbrSpecular.SpecularGlossinessTexture));

                ocasiMaterial.SetValue(MATERIAL_SPECULAR_STRENGTH, extPbrSpecular.GlossinessFactor);
            }

            if (gltfMaterial.ExtAnisotropy)
            {
                auto& extAnisotropy = gltfMaterial.ExtAnisotropy.value();
                ocasiMaterial.SetValue(MATERIAL_ANISOTROPY, extAnisotropy.AnisotropyFactor);
                ocasiMaterial.SetValue(MATERIAL_ANISOTROPY_ROTATION, extAnisotropy.AnisotropyDirection);

                ocasiMaterial.SetValue(MATERIAL_USE_COMBINED_ANISOTROPY_ANISOTROPY_ROTATION_TEXTURE, true);
                ocasiMaterial.SetTexture(MATERIAL_TEXTURE_ANISOTROPY, CreateTexture(extAnisotropy.AnisotropyTexture));
            }

            if (gltfMaterial.ExtClearcoat)
            {
                auto& extClearcoat = gltfMaterial.ExtClearcoat.value();
                ocasiMaterial.SetValue(MATERIAL_CLEARCOAT, extClearcoat.ClearcoatFactor);
                ocasiMaterial.SetTexture(MATERIAL_TEXTURE_CLEARCOAT, CreateTexture(extClearcoat.ClearcoatTexture));

                ocasiMaterial.SetValue(MATERIAL_CLEARCOAT_ROUGHNESS, extClearcoat.ClearcoatRoughnessFactor);
                ocasiMaterial.SetTexture(MATERIAL_TEXTURE_CLEARCOAT_ROUGHNESS, CreateTexture(extClearcoat.ClearcoatRoughnessTexture));

                ocasiMaterial.SetTexture(MATERIAL_TEXTURE_CLEARCOAT_NORMAL, CreateTexture(extClearcoat.ClearcoatNormalTexture));
            }

            // TODO: Iridescence, Volume, Sheen, Transmission

        }
    }

    std::unique_ptr<Image> GLTFImporter::CreateTexture(std::optional<GLTF::TextureInfo>& texInfo)
    {
        if (!texInfo.has_value())
            return nullptr;

        auto& gltfAsset = *m_Asset;

        const GLTF::TextureInfo& gltfInfo = texInfo.value();

        OCASI_ASSERT(gltfInfo.Texture < gltfAsset.Textures.size());
        GLTF::Texture& gltfTexture = gltfAsset.Textures.at(gltfInfo.Texture);

        OCASI_ASSERT_MSG(gltfTexture.Source != INVALID_ID, FORMAT("Do not know what to do with a texture that does not contain an image source. Texture json index: {}", texInfo->Texture));
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
            std::vector<uint8_t> data = GetBufferViewData(gltfImage.BufferView, 0, unused);

            // TODO: Maybe remove this as it is not used
            // When bufferView is defined, mimeType must also be defined
            OCASI_ASSERT(!gltfImage.MimeType.empty());
            ImageType type = ConvertMimeTypeToImagType(gltfImage.MimeType);

            return MakeUnique<Image>(std::move(data), settings);
        }
        else if (!gltfImage.URI.empty())
        {
            std::string uri = Util::URIUnescapedString(gltfImage.URI);
            if (Util::StartsWith(uri, "data:"))
            {
                std::string data = gltfImage.URI.substr(uri.find(':'));
                size_t readSize = 0;
                uint8_t* binaryData = Util::DecodeBase64(data, readSize);
                if (!binaryData)
                    throw FailedImportError("Could not read Base64 encoded string.");
                
                std::vector<uint8_t> binaryDataVector(readSize);

                std::memcpy(binaryDataVector.data(), binaryData, readSize);

                return std::make_unique<Image>(std::move(binaryDataVector), settings);
            }
            else if (Path path = m_FileReader->GetParentPath() / uri; std::filesystem::exists(path))
            {
                return std::make_unique<Image>(path, settings);
            }
            else
            {
                throw FailedImportError("Image URI must either be a valid Base64 encoded string or a valid relative path to an image file.");
            }
        }
        else
        {
            throw FailedImportError("Image must either define a valid buffer view or an URI property.");
        }
    }

    std::vector<uint8_t> GLTFImporter::GetBufferViewData(size_t bufferViewIndex, size_t accessorOffset, size_t& outByteStride)
    {
        auto& asset = *m_Asset;
        OCASI_ASSERT(bufferViewIndex < asset.BufferViews.size());
        auto& bufferView = asset.BufferViews.at(bufferViewIndex);
        outByteStride = bufferView.ByteStride;

        OCASI_ASSERT(bufferView.Buffer < asset.Buffers.size());
        auto& buffer = asset.Buffers.at(bufferView.Buffer);

        return buffer.Get(bufferView.ByteLength, bufferView.ByteOffset + accessorOffset);
    }

    std::vector<uint8_t> GLTFImporter::GetAccessorData(size_t accessorIndex)
    {
        auto& asset = *m_Asset;

        OCASI_ASSERT(accessorIndex < asset.Accessors.size());
        auto& accessor = asset.Accessors.at(accessorIndex);
        OCASI_ASSERT_MSG(accessor.BufferView != INVALID_ID, "Don't know what to do with an accessor that does not contain a buffer view.");

        // This is the accessor offset, not the buffer view offset
        size_t elementSize = GLTF::ComponentTypeToBytes(accessor.CompType) * (size_t) accessor.Type;

        size_t byteStride = 0;
        std::vector<uint8_t> data = GetBufferViewData(accessor.BufferView, accessor.ByteOffset, byteStride);
        OCASI_ASSERT(!data.empty());

        // The byte stride specifies the number of bytes for each element. It is not the element size, but
        // the padding between an element, and it's neighbour including the elements size (stride = element size + padding).
        if (byteStride != 0 && byteStride != elementSize)
        {
            OCASI_ASSERT(byteStride % GLTF::ComponentTypeToBytes(accessor.CompType) == 0);
            for (size_t i = 0; i < accessor.ElementCount; i++)
            {
                std::memcpy(&data[i * elementSize], &data[i * byteStride], elementSize);
            }
            data.resize(elementSize * accessor.ElementCount);
        }

        if (accessor.SparseAccessor.has_value())
        {
            auto& sparse = accessor.SparseAccessor.value();
            OCASI_ASSERT(sparse.Indices.BufferView < asset.BufferViews.size());

            size_t sparseIndicesByteStride; // ignored
            std::vector<uint8_t> sparseIndicesData = GetBufferViewData(sparse.Indices.BufferView, sparse.Indices.ByteOffset, sparseIndicesByteStride);
            OCASI_ASSERT(!sparseIndicesData.empty());

            size_t sparseValuesByteStride; // ignored
            std::vector<uint8_t> sparseValuesData = GetBufferViewData(sparse.Values.BufferView, sparse.Values.ByteOffset, sparseValuesByteStride);
            OCASI_ASSERT(!sparseValuesData.empty());

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
                        std::memcpy(&index, &sparseIndicesData[i * sizeof(uint16_t)], sizeof(uint16_t));
                        break;
                    }
                    case GLTF::ComponentType::UnsignedInt:
                    {
                        std::memcpy(&index, &sparseIndicesData[i * sizeof(uint32_t)], sizeof(uint32_t));
                        break;
                    }
                    default:
                        throw FailedImportError(FORMAT("Unsupported component type used for sparse accessor {}.", (int) sparse.Indices.CompType));
                }

                std::memcpy(&data[index * elementSize], &sparseValuesData[i * elementSize], elementSize);
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
                throw FailedImportError("Image filter option has value None");
        }
        return FilterOption::Linear;
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
                throw FailedImportError(FORMAT("Unsupported primitive type: {}", (int) primitive));
        }
    }
    
    ImageType GLTFImporter::ConvertMimeTypeToImagType(const std::string& mimeType)
    {
        if (mimeType == "image/png")
            return ImageType::PNG;
        else if (mimeType == "image/jpeg")
            return ImageType::JPEG;
        else
            return ImageType::None;
    }
}