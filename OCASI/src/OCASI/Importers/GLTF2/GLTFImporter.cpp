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
        if (auto error = glz::read_json(json, jsonString))
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
                if (buffer.m_Data == nullptr && !found)
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
        uint8_t* data = bReader.Get(chunk.ChunkLength);

        // Remove trailing zeros
        for (uint32_t i = chunk.ChunkLength - 1; i > 0; i--)
        {
            if (data[i] == '\0')
                chunk.ChunkLength--;
        }

        return chunk;
    }

    bool GLTFImporter::CheckBinaryHeader()
    {
        uint8_t* data = new uint8_t[BINARY_HEADER_BYTE_SIZE];
        m_FileReader.GetBytes(data, BINARY_HEADER_BYTE_SIZE);
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
        auto& ocasiScene = *m_Scene;

        for (auto& scene : gltfAsset.Scenes)
        {
            CreateNodes(scene.GetIndex());
        }
    }

    void GLTFImporter::CreateNodes(size_t sceneIndex)
    {
        auto& gltfScene = m_Asset->Scenes.at(sceneIndex);
        auto& gltfAsset = *m_Asset;
        auto& ocasiScene = *m_Scene;

        std::shared_ptr<Node> ocasiRootNode = nullptr;
        if (m_Asset->Scenes.size() > 1)
        {
            auto& node = m_Scene->RootNodes.emplace_back();
            ocasiRootNode = node;
        }

        for (size_t& gltfRootNodeIndex : gltfScene.RootNodes)
        {
            GLTF::Node& gltfRootNode = gltfAsset.Nodes.at(gltfRootNodeIndex);
            auto ocasiNode = std::make_shared<Node>();
            ocasiNode->Parent = ocasiRootNode;

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
            auto childOcasiNode = std::make_shared<Node>();
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
}