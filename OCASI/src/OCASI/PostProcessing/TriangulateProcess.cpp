#include "TriangulateProcess.h"

namespace OCASI {
    
    bool TriangulateProcess::NeedsProcessing(SharedPtr<Scene> scene, SharedPtr<BaseImporter> importer)
    {
        BasePostProcess::NeedsProcessingDefault(this, scene, importer);
        
        return false;
    }
    
    void TriangulateProcess::ExecuteProcess()
    {}
}