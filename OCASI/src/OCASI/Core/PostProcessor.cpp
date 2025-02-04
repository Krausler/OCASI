#include "PostProcessor.h"

namespace OCASI {
    
    std::vector<UniquePtr<BasePostProcess>> PostProcessor::s_PostProcessingProcesses;
    
    void PostProcessor::SetPostProcesses()
    {
        // TODO: Add Processes
        s_PostProcessingProcesses.reserve(5);
    }
    
    PostProcessor::PostProcessor(SharedPtr<Scene> scene, SharedPtr<BaseImporter> importer, PostProcessorOptions options)
        : m_Scene(scene), m_Importer(importer), m_Processes(options)
    {
        if (s_PostProcessingProcesses.empty())
            SetPostProcesses();
    }
    
    void PostProcessor::ExecutePostProcesses()
    {
        for (auto& process : s_PostProcessingProcesses)
        {
            if (m_Processes & process->GetProcessType())
            {
                if(!process->NeedsProcessing(m_Scene, m_Importer))
                    continue;
                
                process->ExecuteProcess();
            }
        }
    }
}