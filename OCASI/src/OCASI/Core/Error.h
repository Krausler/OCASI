#pragma once

#include <exception>
#include <format>

namespace OCASI {
    
    class FailedImportError : public std::runtime_error
    {
    public:
        FailedImportError(const String& msg)
            : std::runtime_error(std::format("OCASI: {}", msg))
        {}
        
        FailedImportError(std::string_view& msg)
            : FailedImportError(std::format("OCASI: {}", msg))
        {}
    };
    
}
