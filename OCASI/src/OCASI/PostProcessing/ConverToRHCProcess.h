#pragma once

#include "OCASI/Core/BasePostProcess.h"

namespace OCASI {
    class Node;
    
    class ConvertToRHCProcess : public BasePostProcess
    {
    public:
        ConvertToRHCProcess() = default;
        ~ConvertToRHCProcess() = default;
        
        virtual bool NeedsProcessing(SharedPtr<Scene> scene, SharedPtr<BaseImporter> importer) override;
        virtual void ExecuteProcess() override;
        
        PostProcessorOptions GetProcessType() const override { return PostProcessorOptions::Triangulate; }
    private:
        void FlipRotation(SharedPtr<Node> node);
    };
    
}