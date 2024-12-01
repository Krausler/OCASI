#pragma once

#include "OCASI/Core/BaseImporter.h"
#include "OCASI/Core/FileUtil.h"
#include "OCASI/Importers/GLTF2/JsonParser.h"

namespace glz {
    class json_t;
}

namespace OCASI {
    class BinaryReader;

    struct GLBHeader
    {
        uint32_t Magic; // binary header expands to "glTF" string
        uint32_t Version; // Is only valid if it equals 2
        uint32_t FileLength; // The spec defines a maximum file size of 4GB, however for simplicity std::numerical_limits<uint32_t>::max() is used
    };

    struct GLBChunk
    {
        uint32_t ChunkLength; // The chunk length in bytes
        uint32_t Type; // Maybe use an enum for this
        uint8_t* Data; // The chunks data with byte length of ChunkLength
    };

    class GLTFImporter : public BaseImporter
    {
    public:
        GLTFImporter(FileReader& reader);

        virtual bool CanLoad() override;
        virtual std::shared_ptr<Scene> Load3DFile() override;
    private:
        bool LoadBinary();
        GLBChunk LoadChunk(BinaryReader& bReader);

        void ConvertToOCASIScene();

        bool CheckBinaryHeader();
        void CreateNodes(size_t sceneIndex);
        void TraverseNodes(GLTF::Node& gltfNode, std::shared_ptr<Node> ocasiNode);
        void CreateMesh(size_t meshIndex);
        std::vector<uint8_t> GetAccessorData(size_t accessorIndex);
        std::vector<uint8_t> GetBufferViewData(size_t bufferViewIndex, size_t accessorOffset, size_t& outByteStride);
    private:
        FileReader& m_FileReader;
        glz::json_t* m_Json = nullptr;

        std::shared_ptr<Scene> m_Scene;
        std::shared_ptr<GLTF::Asset> m_Asset;
    };

}
