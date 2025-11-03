#pragma once

#include "OCASI/Core/Scene.h"

#include "OCBase/IO/FileStream.h"

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
        
        virtual ExpectedImportT<SharedPtr<Scene>> Load3DFile(OCBase::FileStreamReader& reader) = 0;
        virtual bool CanLoad(OCBase::FileStreamReader& reader) = 0;
        
        virtual const Vector<std::string_view> GetSupportedFileExtensions() const = 0;
        virtual ImporterType GetImporterType() const = 0;
        virtual String GetImporterName() const = 0;
        
    protected:
        OCBase::FileStreamReader* m_FileReader = nullptr;
    };
}