#pragma once

#include "OCASI/Core/BasePostProcess.h"

namespace OCASI {
    
    class TriangulateProcess : public BasePostProcess
    {
    public:
        TriangulateProcess() = default;
        ~TriangulateProcess() = default;
        
        virtual bool NeedsProcessing(SharedPtr<Scene> scene, SharedPtr<BaseImporter> importer) override;
        virtual void ExecuteProcess() override;
        
        PostProcessorOptions GetProcessType() const override { return PostProcessorOptions::Triangulate; }
    };
    
}
