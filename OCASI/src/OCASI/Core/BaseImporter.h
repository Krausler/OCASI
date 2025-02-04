#pragma once

#include "OCASI/Core/Scene.h"

#include "OCASI/Core/FileUtil.h"

namespace OCASI {
    
    enum class ImporterType
    {
        None = 0,
        GLTF,
        OBJ
    };

    class BaseImporter 
    {
    public:
        virtual ~BaseImporter() = default;
        
        virtual std::shared_ptr<Scene> Load3DFile(FileReader& reader) = 0;
        virtual bool CanLoad(FileReader& reader) = 0;
        
        virtual std::string_view GetLoggerPattern() const = 0;
        virtual const std::vector<std::string_view> GetSupportedFileExtensions()  const = 0;
        virtual ImporterType GetImporterType() const = 0;
    };
}