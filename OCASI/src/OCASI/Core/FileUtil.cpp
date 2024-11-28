#include "FileUtil.h"

namespace OCASI {

    FileReader::FileReader(const Path &path, bool isBinary)
        : m_Path(path), m_Binary(isBinary), m_FileReader(m_Path, m_Binary ? std::ios::binary : std::ios::in)
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

    uint8_t* FileReader::GetFileDataInBytes(size_t& outSize)
    {
        m_FileReader.unsetf(std::ios::skipws);

        m_FileReader.seekg(0, std::ios::end);
        std::streampos fileSize = outSize = m_FileReader.tellg();
        m_FileReader.seekg(0, std::ios::beg);

        uint8_t* out = new uint8_t[fileSize];
        m_FileReader.read((char*) out, fileSize);

        return out;
    }

    std::string FileReader::GetFileString()
    {
        m_FileReader.unsetf(std::ios::skipws);

        m_FileReader.seekg(0, std::ios::end);
        std::streampos fileSize = m_FileReader.tellg();
        m_FileReader.seekg(0, std::ios::beg);

        std::string out;
        out.resize(fileSize);
        m_FileReader.read(out.data(), fileSize);

        return out;
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