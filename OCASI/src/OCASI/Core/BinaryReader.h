#pragma once

#include "OCASI/Core/FileUtil.h"

namespace OCASI {

    class BinaryReader
    {
    public:
        BinaryReader(FileReader& reader);
        BinaryReader(uint8_t* data, size_t dataSize);
        BinaryReader(std::vector<uint8_t>& data);
        ~BinaryReader();

        template<typename T>
        T GetType()
        {
            OCASI_ASSERT(m_Pointer < m_DataSize);
            T type;
            std::memcpy(&type, m_Data + m_Pointer, sizeof(T));
            m_Pointer += sizeof(T);
            return type;
        }

        uint8_t * Get(size_t size);

        uint8_t GetByte();
        char GetChar();

        // Unsigned integers

        uint16_t GetUint16();
        uint32_t GetUint32();
        uint64_t GetUint64();

        // Signed integers

        int16_t Getint16();
        int32_t GetInt32();
        int64_t GetInt64();

        // Floating point numbers

        float GetFloat();
        double GetDouble();

        size_t GetPointer() const { return m_Pointer; }
        void SetPointer(size_t pointer) { m_Pointer = pointer; }
        void MovePointer(size_t moveAmount) { m_Pointer += moveAmount; }

    private:
        uint8_t* m_Data = nullptr;
        size_t m_DataSize = 0;

        size_t m_Pointer = 0;
    };

}
