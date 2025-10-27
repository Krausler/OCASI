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
    
    struct ImportError
    {
        enum class Type
        {
            Success = 0,
            MissingParameter,
            InvalidParameter,
            ReadMalfunction,
            UnsupportedFeature,
            RequirementsNotMet,
            NoImporterFound,
            Buffer,
            File
        };
        
        Type ErrorCode;
        String ErrorMsg;
        
        String GetMessage() const
        {
            switch (ErrorCode)
            {
                case Type::Success:
                    return "Success";
                case Type::MissingParameter:
                    return FORMAT("ImportError (Type: MissingParameter): {}", ErrorMsg);
                case Type::InvalidParameter:
                    return FORMAT("ImportError (Type: InvalidParameter): {}", ErrorMsg);
                case Type::ReadMalfunction:
                    return FORMAT("ImportError (Type: ReadMalfunction): {}", ErrorMsg);
                case Type::UnsupportedFeature:
                    return FORMAT("ImportError (Type: UnsupportedFeature): {}", ErrorMsg);
                case Type::RequirementsNotMet:
                    return FORMAT("ImportError (Type: RequirementsNotMet): {}", ErrorMsg);
                case Type::NoImporterFound:
                    return FORMAT("ImportError (Type: NoImporterFound): {}", ErrorMsg);
                case Type::Buffer:
                    return FORMAT("ImportError (Type: Buffer): {}", ErrorMsg);
                case Type::File:
                    return FORMAT("ImportError (Type: File): {}", ErrorMsg);
            }
        }
    };
    
    using ExpectedImport = ExpectedVoid<ImportError>;
    template<typename T>
    using ExpectedImportT = Expected<T, ImportError>;
}
