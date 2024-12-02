#include "BinaryReader.h"

namespace OCASI {

    BinaryReader::BinaryReader(FileReader &reader)
        : m_DataSize(reader.GetFileSize())
    {
        m_Data = reader.GetFileDataInBytes();
    }

    BinaryReader::BinaryReader(uint8_t* data, size_t dataSize)
        : m_DataSize(dataSize)
    {
        OCASI_ASSERT(data);
        m_Data = new uint8_t[m_DataSize];
        std::memcpy(m_Data, data, m_DataSize);
    }

    BinaryReader::BinaryReader(std::vector<uint8_t> &data)
        : m_DataSize(data.size())
    {
        OCASI_ASSERT(!data.empty());
        m_Data = new uint8_t[m_DataSize];
        std::memcpy(m_Data, data.data(), m_DataSize);
    }

    BinaryReader::~BinaryReader()
    {
        delete m_Data;
    }

    uint8_t* BinaryReader::Get(size_t size)
    {
        OCASI_ASSERT(m_Pointer < m_DataSize);
        uint8_t* data = new uint8_t[size];
        std::memcpy(data, m_Data + m_Pointer, size);
        m_Pointer += size;
        return data;
    }

    uint8_t BinaryReader::GetByte()
    {
        return GetType<uint8_t>();
    }

    char BinaryReader::GetChar()
    {
        return GetType<char>();
    }

    uint16_t BinaryReader::GetUint16()
    {
        return GetType<uint16_t>();
    }

    uint32_t BinaryReader::GetUint32()
    {
        return GetType<uint32_t>();
    }

    uint64_t BinaryReader::GetUint64()
    {
        return GetType<uint64_t>();
    }

    int16_t BinaryReader::Getint16()
    {
        return GetType<int16_t>();
    }

    int32_t BinaryReader::GetInt32()
    {
        return GetType<int32_t>();
    }

    int64_t BinaryReader::GetInt64()
    {
        return GetType<int64_t>();
    }

    float BinaryReader::GetFloat()
    {
        return GetType<float>();
    }

    double BinaryReader::GetDouble()
    {
        return GetType<double>();
    }
}