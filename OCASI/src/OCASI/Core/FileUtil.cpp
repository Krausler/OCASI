#include "FileUtil.h"

#include "OCASI/Core/StringUtil.h"

namespace OCASI {

    FileReader::FileReader(const Path &path, bool isBinary)
        : m_Path(path), m_Binary(isBinary), m_FileReader(m_Path, std::ios::binary)//m_FileReader(m_Path, m_Binary ? std::ios::binary : std::ios::in)
    {
        m_FileReader.seekg(0, std::ios::end);
        m_FileSize = m_FileReader.tellg();
        m_FileReader.seekg(0, std::ios::beg);
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

    void FileReader::Close()
    {
        m_FileReader.close();
    }

    void FileReader::Reset()
    {
        m_FileReader.clear();
        m_FileReader.seekg(0);
    }

    uint8_t* FileReader::GetFileDataInBytes()
    {
        m_FileReader.unsetf(std::ios::skipws);
        uint8_t* out = new uint8_t[m_FileSize];
        m_FileReader.read((char*) out, m_FileSize);

        return out;
    }

    std::string FileReader::GetFileString()
    {
        m_FileReader.unsetf(std::ios::skipws);
        std::string out;
        out.resize(m_FileSize);
        m_FileReader.read(out.data(), m_FileSize);

        return out;
    }

    std::vector<uint8_t> FileReader::GetBytes(size_t size) {
        m_FileReader.unsetf(std::ios::skipws);
        if (size < m_FileSize)
            return {};

        std::vector<uint8_t > data;
        data.resize(size);
        m_FileReader.read(reinterpret_cast<char*>(data.data()), size);
        return data;
    }

    void FileReader::GetBytes(void* outData, size_t size)
    {
        m_FileReader.unsetf(std::ios::skipws);
        if (size > m_FileSize)
            return;

        m_FileReader.read((char*)outData, (std::streamsize) size);
    }

    void FileReader::Set0()
    {
        m_FileReader.seekg(0, std::ios::beg);
    }
    
    void FileReader::SetBinary()
    {
        m_Binary = true;
        m_FileReader.close();
        m_FileReader.open(m_Path, std::ios::binary);
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
                        if (StartsWith(currentLine, token))
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