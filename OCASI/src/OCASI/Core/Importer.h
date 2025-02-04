#pragma once

#include "OCASI/Core/Scene.h"
#include "OCASI/Core/PostProcessorOptions.h"

namespace OCASI {
    class BaseImporter;
    
    class Importer 
    {
    public:
        static std::shared_ptr<Scene> Load3DFile(const Path& path, PostProcessorOptions options);
        static void SetGlobalPostProcessorOptions(PostProcessorOptions options);
    private:
        static void SetImporters();
    private:
        static std::vector<SharedPtr<BaseImporter>> s_Importers;
        static PostProcessorOptions s_GlobalPostProcessingOptions;
    };
}