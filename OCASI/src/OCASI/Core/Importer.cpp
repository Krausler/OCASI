#include "Importer.h"

#include "OCASI/Importers/OBJ/ObjImporter.h"
#include "OCASI/Importers/GLTF2/GLTFImporter.h"
#include "OCASI/Core/PostProcessor.h"

#include "OCBase/IO/IO.h"
#include "OCBase/IO/FileStream.h"

#include <unordered_map>

namespace OCASI {
    
    Vector<SharedPtr<BaseImporter>> Importer::s_Importers;
    PostProcessorOptions Importer::s_GlobalPostProcessingOptions = PostProcessorOptions::None;

    void Importer::SetImporters()
    {
        s_Importers.reserve(2);
        
        s_Importers.push_back(MakeShared<ObjImporter>());
        s_Importers.push_back(MakeShared<GLTFImporter>());
    }
    
    ExpectedImportT<SharedPtr<Scene>> Importer::Load3DFile(const Path& path, PostProcessorOptions options)
    {
        if (s_Importers.empty())
            SetImporters();
        
        if(!exists(path))
            return nullptr;

        String fExtension = path.extension().string();
        
        OCASI_LOG_INFO("Importing Asset: Name = {}, Path = {}.", path.filename().string(), path.string());
        
        // Getting the model importer by checking for the supported importer file extensions
        SharedPtr<BaseImporter> importer = nullptr;
        for (auto& imp : s_Importers)
        {
            auto& availableExtensions = imp->GetSupportedFileExtensions();
            if (std::find(availableExtensions.begin(), availableExtensions.end(), std::string(fExtension)) != availableExtensions.end())
                importer = imp;
        }
        
        if (!importer)
            return UnexpectedF(ImportError(ImportError::Type::NoImporterFound, FORMAT("Could not find importer supporting the '{}' file extension.", fExtension)));
        
        OCASI_LOG_INFO("Found importer compatible with file extension: Extension = {}, Importer = {}.", path.extension().string(), importer->GetImporterName());
        
        String importerName = importer->GetImporterName();
        
        auto e = OCBase::IO::Open(path, OCBase::FileMode::Read);
        if (!e)
            return UnexpectedF(ImportError(ImportError::Type::File, e.error().GetErrorMessage()));
        OCBase::File& file = e.value();
        OCBase::FileStreamReader reader(file);
        
        if (!importer->CanLoad(reader))
            return UnexpectedF(ImportError(ImportError::Type::RequirementsNotMet, FORMAT("{}: Cannot load file, as it failed to be validated.", importerName)));
        
        OCASI_LOG_INFO("Validating file to be compatible with the importer.");
        
        auto loadError = importer->Load3DFile(reader);
        if (!loadError)
        {
            auto& lError = loadError.error();
            lError.SetImporterName(importerName);
            return UnexpectedF(lError);
        }
        
        file.Close();
        
        auto scene = loadError.value();
        
        OCASI_LOG_INFO("Successfully loaded the asset: Path = {}, Scene = [ Models = {}, Materials = {}]", path.generic_string(), scene->Models.size(), scene->Materials.size());
        
        OCASI_LOG_INFO("");
        
        PostProcessor postProcessor(scene, importer, options | s_GlobalPostProcessingOptions);
        postProcessor.ExecutePostProcesses();
        
        return scene;
    }
    
    void Importer::SetGlobalPostProcessorOptions(PostProcessorOptions options)
    {
        s_GlobalPostProcessingOptions = options;
    }
}
