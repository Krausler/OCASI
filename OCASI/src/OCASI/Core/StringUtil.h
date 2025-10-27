#pragma once

#include "OCASI/Core/Base.h"

namespace OCASI::Util {

    template<class Iterator>
    String GetToNextToken(Iterator& iter, const Iterator& end, std::initializer_list<char> tokens)
    {
        OCASI_ASSERT(iter != end);
        for (auto it = iter; it != end; it++)
        {
            for (char token : tokens)
            {
                if (it == end || *it == token)
                {
                    String result(iter, it);
                    iter = it;
                    return result;
                }
            }
        }
        return {};
    }

    template<class Iterator>
    String GetToNextToken(Iterator& iter, const Iterator& end, char token)
    {
        OCASI_ASSERT(iter != end);
        for (auto it = iter; it != end; it++)
        {
            if (it == end || *it == token)
            {
                String result(iter, it);
                iter = it;
                return result;
            }
        }
        return {};
    }

    template<class Iterator>
    String GetToNextTokenOrEndOfIterator(Iterator& iter, const Iterator& end, char token)
    {
        OCASI_ASSERT(iter != end);

        // Skipping the first token if it matches the searched for token.
        if(*iter == token)
            iter++;

        Iterator it = iter;
        while (true)
        {
            if (it == end || *it == token)
            {
                String result(iter, it);
                iter = it;
                return result;
            }
            it++;
        }
    }

    template<class Iterator>
    String GetToNextSpaceOrEndOfLine(Iterator& iter, const Iterator& end)
    {
        return GetToNextTokenOrEndOfIterator<Iterator>(iter, end, ' ');
    }

    template<class Iterator>
    uint32_t GetAmountOfTokens(const Iterator& iter, const Iterator& end, char token)
    {
        uint32_t numberOfTokens = 0;
        for (auto it = iter; it != end; it++)
        {
            if (*it == token)
                numberOfTokens++;
        }
        return numberOfTokens;
    }
    
    template<typename char_t>
    bool IsLineEnd(char_t c)
    {
        return (c == '\n' || c == '\0' || c == '\f' || c == '\r');
    }

    template<typename char_t>
    bool IsSpace(char_t c)
    {
        return (c == ' ' || c == '\t');
    }

    template<typename char_t>
    bool IsLineEndOrSpace(char_t c)
    {
        return (IsSpace(c) || IsLineEnd(c));
    }

    Vector<std::string> Split(const String& target, char token);
    Vector<std::string> Split(const String& target, char token, uint32_t& outTokenCount);

    uint8_t* DecodeBase64(const String& dataString, size_t& outSize);
    String URIUnescapedString(const String& input);
    bool StartsWith(const String& s, const String& prefix);
}