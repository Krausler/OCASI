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
        bool IsBinary() const { return m_Binary; }
        uint8_t* GetFileDataInBytes(size_t& outSize);
        std::string GetFileString();

        explicit operator bool() const
        {
            return IsOpen();
        }

        Path GetParentPath() { return m_Path.has_parent_path() ? m_Path.parent_path() : ""; }
        Path& GetPath() { return m_Path; }
    private:
        Path m_Path;
        bool m_Binary;
        std::ifstream m_FileReader;
    };

    namespace Util {

        bool FindTokensInFirst100Lines(OCASI::FileReader& reader, const std::vector<std::string>& tokens);

    }

}