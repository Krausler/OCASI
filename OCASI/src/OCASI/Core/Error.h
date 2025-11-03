#pragma once

#include "OCASI/Core/Base.h"

namespace OCASI {
    
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
        String ImporterName;
        
        String GetMessage() const
        {
            switch (ErrorCode)
            {
                case Type::Success:
                    return FORMAT("{}: Success", ImporterName);
                case Type::MissingParameter:
                    return FORMAT("{}: ImportError (Type: MissingParameter): {}", ImporterName, ErrorMsg);
                case Type::InvalidParameter:
                    return FORMAT("{}: ImportError (Type: InvalidParameter): {}", ImporterName, ErrorMsg);
                case Type::ReadMalfunction:
                    return FORMAT("{}: ImportError (Type: ReadMalfunction): {}", ImporterName, ErrorMsg);
                case Type::UnsupportedFeature:
                    return FORMAT("{}: ImportError (Type: UnsupportedFeature): {}", ImporterName, ErrorMsg);
                case Type::RequirementsNotMet:
                    return FORMAT("{}: ImportError (Type: RequirementsNotMet): {}", ImporterName, ErrorMsg);
                case Type::NoImporterFound:
                    return FORMAT("{}: ImportError (Type: NoImporterFound): {}", ImporterName, ErrorMsg);
                case Type::Buffer:
                    return FORMAT("{}: ImportError (Type: Buffer): {}", ImporterName, ErrorMsg);
                case Type::File:
                    return FORMAT("{}: ImportError (Type: File): {}", ImporterName, ErrorMsg);
            }
        }
        
        void SetImporterName(const String& name)
        {
            ImporterName = name;
        }
    };
    
    using ExpectedImport = ExpectedVoid<ImportError>;
    template<typename T>
    using ExpectedImportT = Expected<T, ImportError>;
}
