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
}