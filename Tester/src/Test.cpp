#include "OCASI/Core/Logger.h"
#include "OCASI/Core/StringUtil.h"

#include <iostream>

int main()
{
    OCASI::Logger::Init();
    
    auto ss = OCASI::Split("27//2/  ", '/');

    for(auto s : ss)
        std::cout << "'" << s << "'" << std::endl;
}