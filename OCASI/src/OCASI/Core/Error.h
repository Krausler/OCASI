#pragma once

#include "OCASI/Core/Logger.h"

#include <exception>

namespace OCASI {
    
    class FailedImportError : public std::runtime_error
    {
    public:
        FailedImportError(const std::string& msg)
            : std::runtime_error(msg)
        {}
        
        FailedImportError(std::string_view& msg)
            : FailedImportError(std::string (msg))
        {}
    };
    
}
