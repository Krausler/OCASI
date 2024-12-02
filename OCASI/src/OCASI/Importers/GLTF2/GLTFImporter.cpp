#include "GLTFImporter.h"

#include "OCASI/Core/BinaryReader.h"

#include "glm/gtc/quaternion.hpp"
#include "glaze/glaze.hpp"

namespace OCASI {

    const size_t BINARY_HEADER_BYTE_SIZE = 12;
    const uint32_t BINARY_HEADER_MAGIC_VALUE = 0x46546C67;
    const uint32_t BINARY_HEADER_VERSION = 2;

    const uint32_t CHUNK_TYPE_JSON = 0x4E4F534A;
    const uint32_t CHUNK_TYPE_BINARY = 0x004E4942;

    GLTFImporter::GLTFImporter(FileReader &reader)
        : m_FileReader(reader)
    {}

    bool GLTFImporter::CanLoad()
    {
        if (m_FileReader.IsBinary())
        {
            if (m_FileReader.GetFileSize() >= std::numeric_limits<uint32_t>::max())
                return false;

            if (CheckBinaryHeader())
                return true;
        }
        else
        {
            m_Json = new glz::json_t;
            if (auto error = glz::read_json(m_Json, m_FileReader.GetFileString()); !error)
                return true;
        }
        return false;
    }

    std::shared_ptr<Scene> GLTFImporter::Load3DFile()
    {
        std::shared_ptr<GLTF::Asset> asset = nullptr;
        if (m_FileReader.IsBinary())
        {
            if (!LoadBinary())
                return nullptr;
        }
        else
        {
            GLTF::JsonParser parser(m_FileReader, m_Json);
            if (!(m_Asset = parser.ParseGLTFTextFile()))
                return nullptr;
        }

        ConvertToOCASIScene();

        return m_Scene;
    }

    bool GLTFImporter::LoadBinary()
    {
        BinaryReader bReader(m_FileReader);
        // Skip the first 12 bytes including the header, as it has already been checked
        // to be valid in the CheckBinaryHeader function
        bReader.SetPointer(BINARY_HEADER_BYTE_SIZE);

        GLBChunk jsonChunk = LoadChunk(bReader);
        if (jsonChunk.Type != CHUNK_TYPE_JSON)
        {
            OCASI_FAIL("First binary chunk of GLB file must be a json chunk.");
            return false;
        }

        glz::json_t* json = new glz::json_t;
        std::string jsonString((char*)jsonChunk.Data, jsonChunk.ChunkLength);
        if (auto error = glz::read_json(json, jsonString); error)
        {
            OCASI_FAIL(FORMAT("Failed to load json: {}", error.custom_error_message));
            return false;
        }
        GLTF::JsonParser parser(m_FileReader, json);
        if (!(m_Asset = parser.ParseGLTFTextFile()))
            return false;

        // Checking whether there is a second buffer chunk:
        // 2 * chunk info + minimum buffers size (data needs to be aligned to 4)
        constexpr size_t byteSizeToAdd = sizeof(uint32_t) * 2 + 4;
        if (bReader.GetPointer() + byteSizeToAdd < m_FileReader.GetFileSize())
        {
            GLBChunk bufferChunk = LoadChunk(bReader);
            if (jsonChunk.Type != CHUNK_TYPE_JSON)
            {
                OCASI_FAIL("First binary chunk of GLB file must be a json chunk.");
                return false;
            }

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
                    OCASI_FAIL("GlB file has more then one binary chunk defined in it's json definition.");
                    return false;
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
        m_FileReader.GetBytes(data, BINARY_HEADER_BYTE_SIZE);
        m_FileReader.Set0();
        BinaryReader bReader(data, BINARY_HEADER_BYTE_SIZE);
        GLBHeader header = bReader.GetType<GLBHeader>();

        if (!(header.Magic == BINARY_HEADER_MAGIC_VALUE && header.Version == BINARY_HEADER_VERSION && header.FileLength == m_FileReader.GetFileSize()))
            return false;
        return true;
    }

    void GLTFImporter::ConvertToOCASIScene()
    {
        m_Scene = std::make_shared<Scene>();

        auto& gltfAsset = *m_Asset;

        for (auto& gltfScene : gltfAsset.Scenes)
        {
            CreateNodes(gltfScene.GetIndex());
        }

        for (auto& gltfMesh : gltfAsset.Meshes)
        {
            CreateMesh(gltfMesh.GetIndex());
        }

        for (auto& gltfMaterial : gltfAsset.Materials)
        {

        }
    }

    void GLTFImporter::CreateNodes(size_t sceneIndex)
    {
        auto& gltfAsset = *m_Asset;
        auto& ocasiScene = *m_Scene;
        auto& gltfScene = gltfAsset.Scenes.at(sceneIndex);

        // When there are multiple scenes, each scene has a
        std::shared_ptr<Node> ocasiRootNode = nullptr;
        if (m_Asset->Scenes.size() > 1)
        {
            auto& node = m_Scene->RootNodes.emplace_back();
            node->Name = FORMAT("Additional root node for scene: {} (index: {})", gltfScene.Name, sceneIndex);
            ocasiRootNode = node;
        }

        for (size_t& gltfRootNodeIndex : gltfScene.RootNodes)
        {
            GLTF::Node& gltfRootNode = gltfAsset.Nodes.at(gltfRootNodeIndex);
            auto ocasiNode = std::make_shared<Node>();
            ocasiScene.RootNodes.push_back(ocasiNode);

            if (ocasiRootNode)
            {
                ocasiNode->Parent = ocasiRootNode;
                ocasiRootNode->Children.push_back(ocasiNode);
            }

            if (gltfRootNode.Mesh.has_value())
                ocasiNode->MeshIndex = gltfRootNode.Mesh.value();
            if (gltfRootNode.TrsComponent.has_value())
            {
                glm::mat4 translation = glm::translate(translation, gltfRootNode.TrsComponent->Translation);
                glm::mat4 rotation = glm::mat4_cast(gltfRootNode.TrsComponent->Rotation);
                glm::mat4 scale = glm::translate(translation, gltfRootNode.TrsComponent->Scale);

                ocasiNode->LocalTransform = translation * rotation * scale;
            }

            TraverseNodes(gltfRootNode, ocasiNode);
        }
    }

    void GLTFImporter::TraverseNodes(GLTF::Node& gltfNode, std::shared_ptr<Node> ocasiNode)
    {
        for (size_t child : gltfNode.Children)
        {
            auto& childGltfNode = m_Asset->Nodes.at(child);

            std::shared_ptr<Node> childOcasiNode = std::make_shared<Node>();
            childOcasiNode->Parent = ocasiNode;
            ocasiNode->Children.push_back(childOcasiNode);

            if (childGltfNode.Mesh.has_value())
                childOcasiNode->MeshIndex = gltfNode.Mesh.value();
            if (childGltfNode.TrsComponent.has_value())
            {
                glm::mat4 translation = glm::translate(translation, childGltfNode.TrsComponent->Translation);
                glm::mat4 rotation = glm::mat4_cast(childGltfNode.TrsComponent->Rotation);
                glm::mat4 scale = glm::translate(translation, childGltfNode.TrsComponent->Scale);

                childOcasiNode->LocalTransform = translation * rotation * scale;
            }

            if (childGltfNode.LocalTranslationMatrix.has_value())
                childOcasiNode->LocalTransform = childGltfNode.LocalTranslationMatrix.value();

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
            ocasiMesh.MaterialIndex = gltfPrimitive.MaterialIndex.has_value() ? gltfPrimitive.MaterialIndex.value() : INVALID_ID;

            for (auto& [attributeName, accessor] : gltfPrimitive.Attributes)
            {
                // TODO: Currently the data types are fixed, make these dynamic
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
                else if (attributeName.starts_with("TEXCOORD_"))
                {
                    const size_t TEX_COORD_STRING_INDEX = 9;
                    size_t texCoordIndex = std::atoi(&attributeName.at(TEX_COORD_STRING_INDEX));
                    OCASI_ASSERT(texCoordIndex < ocasiMesh.TexCoords.size());
                    auto& texCoords = ocasiMesh.TexCoords.at(texCoordIndex);

                    std::vector<uint8_t> data = GetAccessorData(accessor);
                    OCASI_ASSERT(!data.empty() && data.size() % sizeof(glm::vec2) == 0);

                    texCoords.resize(data.size() / sizeof(glm::vec2));
                    std::memcpy(texCoords.data(), data.data(), data.size());
                }
                else if (attributeName.starts_with("COLOR_"))
                {
                    const size_t COLOR_STRING_INDEX = 6;
                    size_t colorIndex = std::atoi(&attributeName.at(COLOR_STRING_INDEX));

                    std::vector<uint8_t> data = GetAccessorData(accessor);
                    OCASI_ASSERT(!data.empty() && data.size() % sizeof(glm::vec4) == 0);

                    ocasiMesh.VertexColours.resize(data.size() / sizeof(glm::vec4));
                    std::memcpy(ocasiMesh.VertexColours.data(), data.data(), data.size());
                }
            }
        }
    }

    std::vector<uint8_t> GLTFImporter::GetAccessorData(size_t accessorIndex)
    {
        auto& asset = *m_Asset;

        OCASI_ASSERT(accessorIndex < asset.Accessors.size());
        auto& accessor = asset.Accessors.at(accessorIndex);
        OCASI_ASSERT_MSG(accessor.BufferView.has_value(), "Don't know what to do with an accessor that does not contain a buffer view.");

        // This is the accessor offset not the buffer view offset
        size_t elementSize = GLTF::ComponentTypeToBytes(accessor.ComponentType) * (size_t) accessor.DataType;

        size_t byteStride = 0;
        std::vector<uint8_t> data = GetBufferViewData(accessor.BufferView.value(), accessor.ByteOffset, byteStride);
        OCASI_ASSERT(!data.empty());

        // The byte stride specifies the amount of bytes for each element. It is not the element size, but
        // the padding between an element, and it's neighbour including the elements size. stride = (element size + padding)
        if (byteStride != 0 && byteStride != elementSize)
        {
            OCASI_ASSERT(byteStride % GLTF::ComponentTypeToBytes(accessor.ComponentType) == 0);
            for (size_t i = 0; i < accessor.ElementCount; i++)
            {
                std::memcpy(&data[i * elementSize], &data[i * byteStride], elementSize);
            }
        }

        if (accessor.Sparse.has_value())
        {
            auto& sparse = accessor.Sparse.value();
            OCASI_ASSERT(sparse.Indices.BufferView < asset.BufferViews.size());

            size_t sparseIndicesByteStride; // ignored
            std::vector<uint8_t> sparseIndicesData = GetBufferViewData(sparse.Indices.BufferView, sparse.Indices.ByteOffset, sparseIndicesByteStride);
            OCASI_ASSERT(!sparseIndicesData.empty());

            size_t sparseValuesByteStride; // ignored
            std::vector<uint8_t> sparseValuesData = GetBufferViewData(sparse.Values.BufferView, sparse.Values.ByteOffset, sparseValuesByteStride);
            OCASI_ASSERT(!sparseValuesData.empty());

            // HACK: For indexing a conversion from bytes to a number is needed
            for (size_t i = 0; i < sparse.ElementCount; i++)
            {
                uint32_t index = 0;
                switch(sparse.Indices.ComponentType)
                {
                    case GLTF::ComponentType::Short:
                    case GLTF::ComponentType::UnsignedShort:
                    {
                        std::memcpy(&index, &sparseIndicesData[i], sizeof(uint16_t));
                        break;
                    }
                    case GLTF::ComponentType::UnsignedInt:
                    {
                        std::memcpy(&index, &sparseIndicesData[i], sizeof(uint32_t));
                        break;
                    }
                    default:
                        OCASI_FAIL(FORMAT("Invalid component type for sparse indices: {}", (size_t) sparse.Indices.ComponentType));
                }

                std::memcpy(&data[index * elementSize], &sparseValuesData[i * elementSize], elementSize);
            }
        }

        return data;
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
}