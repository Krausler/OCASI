#include "FileUtil.h"

namespace OCASI {

    FileReader::FileReader(const Path &path)
        : m_Path(path), m_FileReader(m_Path)
    {
    }

    FileReader::~FileReader() {
        if (IsOpen())
            Close();
    }

    bool FileReader::NextLine(std::string& outLine)
    {
        return std::getline(m_FileReader, outLine).operator bool();
    }

    bool FileReader::NextLineC(std::vector<char>& outLine)
    {
        std::string line;
        bool r = std::getline(m_FileReader, line).operator bool();
        outLine.resize(line.size());
        outLine.assign(line.begin(), line.end());
        return r;
    }

    void FileReader::Close() {
        m_FileReader.close();
    }

    void FileReader::Reset()
    {
        m_FileReader.clear();
        m_FileReader.seekg(0);
    }

    namespace Util {

        bool FindTokensInFirst100Lines(OCASI::FileReader& reader, const std::vector<std::string>& tokens)
        {
            std::string currentLine;
            for (size_t i = 0; i < 100 && !reader.HasReachedEOF(); i++)
            {
                if(reader.NextLine(currentLine))
                {
                    for (auto& token : tokens)
                    {
                        if (currentLine.starts_with(token))
                        {
                            reader.Reset();
                            return true;
                        }
                    }
                }
            }
            return false;
        }
    }
}