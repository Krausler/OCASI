#include "Importer.h"

#include "OCASI/Importers/OBJ/ObjImporter.h"
#include "OCASI/Importers/GLTF2/GLTFImporter.h"
#include "OCASI/Core/PostProcessor.h"

#include <unordered_map>

#define CAN_LOAD(x, fName) if(!x.CanLoad()) { OCASI_LOG_ERROR("Can't load file as CanLoad() for {} did not succeed.", fName); return nullptr; }

namespace OCASI {
    
    std::vector<SharedPtr<BaseImporter>> Importer::s_Importers;
    PostProcessorOptions Importer::s_GlobalPostProcessingOptions = PostProcessorOptions::ConvertToRHC;

    void Importer::SetImporters()
    {
        s_Importers.reserve(2);
        
        s_Importers.push_back(MakeShared<ObjImporter>());
        s_Importers.push_back(MakeShared<GLTFImporter>());
    }

    std::shared_ptr<Scene> Importer::Load3DFile(const Path& path, PostProcessorOptions options)
    {
        if (s_Importers.empty())
            SetImporters();
        
        if(!exists(path))
        {
            OCASI_FAIL("Requested file does not exist. Verify the 3D model path.");
            return nullptr;
        }

        std::string fExtension = path.extension().string();
        std::shared_ptr<Scene> result = nullptr;

        try
        {
            // Getting the model importer by checking for the supported importer file extensions
            SharedPtr<BaseImporter> importer = nullptr;
            for (auto& imp : s_Importers)
            {
                auto availableExtensions = imp->GetSupportedFileExtensions();
                if (std::find(availableExtensions.begin(), availableExtensions.end(), std::string(fExtension)) != availableExtensions.end())
                    importer = imp;
            }
            
            if (!importer)
                throw FailedImportError(FORMAT("Could not find importer supporting {} file extension.", fExtension));
            
            Logger::SetFileFormatPattern(std::string(importer->GetLoggerPattern()));
            
            FileReader reader(path);
            
            if (!importer->CanLoad(reader))
                throw FailedImportError("Cannot load file, as it failed to be validated.");
            
            result = importer->Load3DFile(reader);
            
            PostProcessor postProcessor(result, importer, options | s_GlobalPostProcessingOptions);
            postProcessor.ExecutePostProcesses();
            
            Logger::ResetPattern();
        }
        catch (const FailedImportError& e)
        {
            Logger::ResetPattern();
            OCASI_LOG_ERROR("Failed to load {}: {}", path.string(), e.what());
            result = nullptr;
        }
        Logger::ResetPattern();
        
        OCASI_ASSERT(result);

        return result;
    }
    
    void Importer::SetGlobalPostProcessorOptions(PostProcessorOptions options)
    {
        s_GlobalPostProcessingOptions = options;
    }
}
