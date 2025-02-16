#pragma once

#include "OCASI/Core/BasePostProcess.h"

namespace OCASI {
    
    class GenerateNormalsProcess : public BasePostProcess
    {
    public:
        GenerateNormalsProcess() = default;
        ~GenerateNormalsProcess() = default;
        
        virtual bool NeedsProcessing(SharedPtr<Scene> scene, SharedPtr<BaseImporter> importer) override;
        virtual void ExecuteProcess() override;
        virtual PostProcessorOptions GetProcessType() const override { return PostProcessorOptions::GenerateNormals; }
    private:
        // Storing the index of the model along with its mesh index
        std::vector<std::pair<size_t, size_t>> m_ModelsWithProcessingNeed;
    };
    
}
