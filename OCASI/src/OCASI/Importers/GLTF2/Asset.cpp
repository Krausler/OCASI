#include "Asset.h"

#include "OCASI/Core/StringUtil.h"

namespace OCASI::GLTF {

    Buffer::Buffer(size_t id, size_t bufferSize)
        : Object(id), m_ByteSize(bufferSize)
    {
    }

    Buffer::Buffer(size_t id, FileReader& reader, size_t bufferSize)
        : Object(id), m_ByteSize(bufferSize)
    {
        if (!reader.IsOpen())
        {
            OCASI_FAIL(FORMAT("glTF binary data file does not exist: {}", reader.GetPath().string()));
            return;
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

    bool Buffer::SetPointer(size_t position)
    {
        if (m_ByteSize <= position)
        {
            OCASI_FAIL("Tried to set glTF Buffer data pointer out of range");
            return false;
        }

        m_Pointer = position;
        return true;
    }

    bool Buffer::AddToPointer(size_t position)
    {
        if (m_ByteSize <= m_Pointer + position)
        {
            OCASI_FAIL("Tried to move glTF Buffer data pointer out of range");
            return false;
        }

        m_Pointer += position;
        return true;
    }

    void* Buffer::Get(size_t byteLength)
    {
        if (m_Pointer + byteLength >= m_ByteSize)
        {
            OCASI_FAIL("Tried to read glTF buffer data that was out of range");
            return nullptr;
        }

        return (void*) (m_Data + m_Pointer);
    }
}
