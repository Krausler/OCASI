#include "PostProcessor.h"

#include "OCASI/PostProcessing/ConverToRHCProcess.h"
#include "OCASI/PostProcessing/TriangulateProcess.h"
#include "OCASI/PostProcessing/GenerateNormalsProcess.h"

namespace OCASI {
    
    std::vector<UniquePtr<BasePostProcess>> PostProcessor::s_PostProcessingProcesses;
    
    void PostProcessor::SetPostProcesses()
    {
        // TODO: Add Processes
        s_PostProcessingProcesses.reserve(5);
        
        s_PostProcessingProcesses.push_back(MakeUnique<ConvertToRHCProcess>());
        s_PostProcessingProcesses.push_back(MakeUnique<TriangulateProcess>());
        s_PostProcessingProcesses.push_back(MakeUnique<GenerateNormalsProcess>());
    }
    
    PostProcessor::PostProcessor(SharedPtr<Scene> scene, SharedPtr<BaseImporter> importer, PostProcessorOptions options)
        : m_Scene(scene), m_Importer(importer), m_Processes(options)
    {
        if (s_PostProcessingProcesses.empty())
            SetPostProcesses();
    }
    
    void PostProcessor::ExecutePostProcesses()
    {
        if (m_Processes == PostProcessorOptions::None)
            return;
        
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