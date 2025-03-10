#include "Asset.h"

#include "OCASI/Core/StringUtil.h"

namespace OCASI::GLTF {

    size_t ComponentTypeToBytes(ComponentType type) {
        switch (type) {
            case ComponentType::None:
                return INVALID_ID;
            case ComponentType::Byte:
            case ComponentType::UnsignedByte:
                return 1;
            case ComponentType::Short:
            case ComponentType::UnsignedShort:
                return 2;
            case ComponentType::UnsignedInt:
            case ComponentType::Float:
                return 4;
        }
        return INVALID_ID;
    }

    Buffer::Buffer(size_t id, size_t bufferSize)
        : Object(id), m_ByteSize(bufferSize)
    {
    }

    Buffer::Buffer(size_t id, FileReader& reader, size_t bufferSize)
        : Object(id), m_ByteSize(bufferSize)
    {
        if (!reader.IsOpen())
        {
            throw FailedImportError(FORMAT("Cannot open .bin file at location {}", reader.GetPath().string()));
        }

        size_t fileSize = reader.GetFileSize();
        m_Data = reader.GetFileDataInBytes();

        OCASI_ASSERT_MSG(fileSize == bufferSize, FORMAT("Specified byte size doe not match read byte size of glTF .bin file data. read size: {}, specified size: {}", fileSize, bufferSize));
        m_ByteSize = fileSize;
    }

    Buffer::Buffer(size_t id, const std::string& URIData, size_t bufferSize)
        : Object(id)
    {
        size_t readSize = 0;
        m_Data = Util::DecodeBase64(URIData, readSize);

        OCASI_ASSERT_MSG(readSize == bufferSize, FORMAT("Specified byte size doe not match read byte size of glTF uri base64 encoded data. read size: {}, specified size: {}", readSize, bufferSize));
        m_ByteSize = readSize;
    }

    Buffer::~Buffer()
    {
        delete m_Data;
    }

    std::vector<uint8_t> Buffer::Get(size_t byteLength, size_t offset)
    {
        if (offset + byteLength > m_ByteSize)
            throw FailedImportError("Cannot read data that lies outside the buffers memory.");
        
        std::vector<uint8_t> result;
        result.resize(byteLength);
        void* data = m_Data + offset;
        std::memcpy(result.data(), data, byteLength);

        return result;
    }
}
