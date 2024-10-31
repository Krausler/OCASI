#pragma once

#include "OCASI/Core/Base.h"

#include <fstream>

namespace OCASI {

    class FileReader
    {
    public:
        FileReader(const Path& path);
        ~FileReader();

        bool NextLine(std::string& outLine);
        bool NextLineC(std::vector<char>& outChars);
        void Close();
        void Reset();
        bool HasReachedEOF() const { return m_FileReader.eof(); }
        bool IsOpen() const { return m_FileReader.is_open(); }

        explicit operator bool() const
        {
            return IsOpen();
        }

        Path& GetPath() { return m_Path; }
    private:
        Path m_Path;
        std::ifstream m_FileReader;
    };

    namespace Util {

        bool FindTokensInFirst100Lines(OCASI::FileReader& reader, const std::vector<std::string>& tokens);

    }

}