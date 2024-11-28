#include "StringUtil.h"

namespace OCASI::Util {

    std::vector<std::string> Split(const std::string& target, char token)
    {
        std::vector<std::string> result;
        uint32_t lastIndex = 0;
        for (uint32_t i = 0; i < target.size(); i++)
        {
            if(target[i] == token)
            {
                if (std::string s = target.substr(lastIndex, i - lastIndex); !s.empty())
                    result.push_back(s);
                lastIndex = i + 1;
            }
        }

        if(lastIndex < target.size())
        {
            result.push_back(target.substr(lastIndex));
        }

        return result;
    }

    std::vector<std::string> Split(const std::string& target, char token, uint32_t& outTokenCount)
    {
        std::vector<std::string> result;
        uint32_t lastIndex = 0;
        outTokenCount = 0;
        for (uint32_t i = 0; i < target.size(); i++)
        {
            if(target[i] == token)
            {
                outTokenCount++;
                if (std::string s = target.substr(lastIndex, i - lastIndex); !s.empty())
                    result.push_back(s);
                lastIndex = i + 1;
            }
        }

        if(lastIndex < target.size())
        {
            result.push_back(target.substr(lastIndex));
        }

        return result;
    }


    uint8_t* DecodeBase64(const std::string &dataString, size_t& outSize)
    {
        // Base 64 splits binary data into 3 bytes (24 bits) each which then get decomposed into groups of 4 (6 bits each)
        constexpr size_t BASE64_BYTE_CHUNK_SIZE = 3;
        constexpr size_t BASE64_GROUP_COUNT_PER_CHUNK = 4;
        constexpr size_t BASE64_BIT_GROUP_SIZE = 6;

        constexpr unsigned char base64DecodingTable[] = {
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 0-15
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 16-31
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, // 32-47
                52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 0, 64, 64,  // 48-63
                64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,           // 64-79
                15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64, // 80-95
                64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 96-111
                41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64, // 112-127
        };

        if (dataString.size() % BASE64_GROUP_COUNT_PER_CHUNK > 0)
        {
            OCASI_FAIL("Failed to decode base64 data. Data is not aligned with 3 bytes.")
            return nullptr;
        }

        size_t dataSize = dataString.size();
        size_t base64OutputByteSize = dataSize;

        // Checking for padding character at the end the buffer
        if (dataString.at(dataSize - 1 ) == '=')
            base64OutputByteSize--;
        if (dataString.at(dataSize - 2) == '=')
            base64OutputByteSize--;

        uint8_t* out = new uint8_t[base64OutputByteSize];

        size_t byteCounter = 0;
        for (int i = 0; i < dataSize;)
        {
            // Extract the binary data representation of each chunk of 6 bits. If the current processed character is a padding character 0 otherwise
            // it gets decoded an AND operation is performed for each of the read in data to clear the first bit of the ASCII decoding as ASCII only
            // uses 7 bits to identify characters (Some random binary value: 10101010 & 01111111 (0x7F) = 00101010)
            uint32_t bitGroup1 = dataString[i] == '=' ? 0 : base64DecodingTable[dataString.at(i)] & 0x7F;
            i++;
            uint32_t bitGroup2 = dataString[i] == '=' ? 0 : base64DecodingTable[dataString.at(i)] & 0x7F;
            i++;
            uint32_t bitGroup3 = dataString[i] == '=' ? 0 : base64DecodingTable[dataString.at(i)] & 0x7F;
            i++;
            uint32_t bitGroup4 = dataString[i] == '=' ? 0 : base64DecodingTable[dataString.at(i)] & 0x7F;
            i++;

            uint32_t quatrouple = (bitGroup1 << 18) | (bitGroup2 << 12) | (bitGroup3 << 6) | bitGroup4;

            if (byteCounter < base64OutputByteSize)
                out[byteCounter++] = (quatrouple >> 16) | 0xFF;
            if (byteCounter < base64OutputByteSize)
                out[byteCounter++] = (quatrouple >> 8) | 0xFF;
            if (byteCounter < base64OutputByteSize)
                out[byteCounter++] = quatrouple | 0xFF;
        }

        outSize = byteCounter;

        return out;
    }

}

