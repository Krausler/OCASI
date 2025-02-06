#pragma once

#include "OCASI/Core/Base.h"

#include "OCASI/Core/PostProcessorOptions.h"

namespace OCASI {
    struct Scene;
    class BaseImporter;
    
    class BasePostProcess
    {
    public:
        static void NeedsProcessingDefault(BasePostProcess* process, SharedPtr<Scene> scene, SharedPtr<BaseImporter> importer)
        {
            OCASI_ASSERT(process && scene && importer);
            process->m_Scene = scene;
            process->m_Importer = importer;
        }
    public:
        BasePostProcess() = default;
        virtual ~BasePostProcess() = default;
        
        virtual bool NeedsProcessing(SharedPtr<Scene> scene, SharedPtr<BaseImporter> importer) = 0;
        virtual void ExecuteProcess() = 0;
        
        virtual PostProcessorOptions GetProcessType() const = 0;
    protected:
        SharedPtr<Scene> m_Scene = nullptr;
        SharedPtr<BaseImporter> m_Importer = nullptr;
    };
    
}
