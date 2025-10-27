#include "Asset.h"

#include "OCASI/Core/StringUtil.h"

namespace OCASI::GLTF {

    Buffer::Buffer(size_t id, size_t bufferSize)
        : Object(id), m_ByteSize(bufferSize)
    {
    }

    Buffer::Buffer(size_t id, OCBase::FileStreamReader& reader, size_t bufferSize)
        : Object(id), m_ByteSize(bufferSize)
    {
        OCASI_ASSERT(reader.GetFile().IsOpen());

        size_t fileSize = reader.GetFile().GetSize();
        reader.SetOffset(0);
        m_Data = reader.Read(bufferSize);

        OCASI_ASSERT(fileSize == bufferSize, "Specified byte size does not match read byte size of glTF .bin file data. read size: {}, specified size: {}", fileSize, bufferSize);
        m_ByteSize = fileSize;
    }

    Buffer::Buffer(size_t id, const String& URIData, size_t bufferSize)
        : Object(id)
    {
        size_t readSize = 0;
        m_Data = Util::DecodeBase64(URIData, readSize);

        OCASI_ASSERT(readSize == bufferSize, "Specified byte size doe not match read byte size of glTF uri base64 encoded data. read size: {}, specified size: {}", readSize, bufferSize);
        m_ByteSize = readSize;
    }

    Buffer::~Buffer()
    {
        delete m_Data;
    }

    ExpectedImportT<std::span<uint8_t>> Buffer::Get(size_t byteLength, size_t offset)
    {
        if (offset + byteLength > m_ByteSize)
            return UnexpectedF(ImportError(ImportError::Type::Buffer, FORMAT("The specified byte range exceeds the buffers length. Buffer = [{}]", GetIndex())));
        
        void* byteData = m_Data + offset;
        std::span<uint8_t> data((uint8_t*)byteData, byteLength);

        return data;
    }
}
