#pragma once

#include "OCASI/Core/Base.h"

#include <fstream>

namespace OCASI {

    class FileReader
    {
    public:
        FileReader(const Path& path, bool isBinary = false);
        ~FileReader();

        bool NextLine(std::string& outLine);
        bool NextLineC(std::vector<char>& outChars);
        void Close();
        void Reset();
        bool HasReachedEOF() const { return m_FileReader.eof(); }
        bool IsOpen() const { return m_FileReader.is_open(); }
        uint8_t* GetFileDataInBytes();
        std::string GetFileString();
        std::vector<uint8_t> GetBytes(size_t size);
        void GetBytes(void* outData, size_t size);
        void Set0();

        template<typename T>
        T GetBytes()
        {
            auto data = GetBytes(sizeof(T));
            T out;
            std::memcpy(&out, data.data(), sizeof(T));
            return out;
        }

        explicit operator bool() const
        {
            return IsOpen();
        }

        Path GetParentPath() { return m_Path.has_parent_path() ? m_Path.parent_path() : ""; }
        Path& GetPath() { return m_Path; }
        size_t& GetFileSize() { return m_FileSize; }
        bool IsBinary() const { return m_Binary; }

    private:
        Path m_Path;
        bool m_Binary;
        std::ifstream m_FileReader;
        size_t m_FileSize;
    };
    
    namespace Util {
        
        bool StartsWith(const std::string& s, const std::string& prefix);
        bool FindTokensInFirst100Lines(OCASI::FileReader& reader, const std::vector<std::string>& tokens);
        
    }

}