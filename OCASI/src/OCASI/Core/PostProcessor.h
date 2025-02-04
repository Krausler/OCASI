#pragma once

#include "OCASI/Core/Base.h"

#include "OCASI/Core/PostProcessorOptions.h"
#include "OCASI/Core/BasePostProcess.h"

namespace OCASI {
    struct Scene;
    class BaseImporter;
    
    class PostProcessor
    {
    private:
        static void SetPostProcesses();
    public:
        PostProcessor(SharedPtr<Scene> scene, SharedPtr<BaseImporter> importer, PostProcessorOptions options);
        void ExecutePostProcesses();
    private:
        static std::vector<UniquePtr<BasePostProcess>> s_PostProcessingProcesses;
    private:
        SharedPtr<Scene> m_Scene = nullptr;
        SharedPtr<BaseImporter> m_Importer = nullptr;
        PostProcessorOptions m_Processes = PostProcessorOptions::None;
    };
    
}