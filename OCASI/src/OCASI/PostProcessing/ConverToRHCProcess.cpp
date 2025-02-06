#include "ConverToRHCProcess.h"

#include "OCASI/Core/BaseImporter.h"

namespace OCASI {
    
    bool ConvertToRHCProcess::NeedsProcessing(SharedPtr<Scene> scene, SharedPtr<BaseImporter> importer)
    {
        BasePostProcess::NeedsProcessingDefault(this, scene, importer);
        
        switch (importer->GetImporterType())
        {
            case ImporterType::OBJ:
                return false;
            case ImporterType::GLTF:
                return true;
            default:
                throw FailedImportError("Invalid importer type.");
        }
    }
    
    void ConvertToRHCProcess::ExecuteProcess()
    {
        auto& models = m_Scene->Models;
        
        for (auto& model : models)
        {
            for (auto& mesh : model.Meshes)
            {
                // Flipping the vertices and normal z component, as in an RHC, the z axis points in the opposite
                // direction of an LHC
                for (auto& vertex : mesh.Vertices)
                    vertex.z *= -1;
                
                for (auto& normal : mesh.Normals)
                    normal.z *= -1;
                
                // Flipping the winding order. A right-handed coordinate system uses a counter-clockwise
                // processing order
                size_t triangleCount = mesh.Indices.size() / 3;
                for (size_t i = 0; i < triangleCount; i++)
                {
                    size_t index = i * 3;
                    
                    std::swap(mesh.Indices.at(index + 0), mesh.Indices.at(index + 2));
                }
            }
        }
        
        // Flipping the rotations
        for (auto& rootNodes : m_Scene->RootNodes)
            FlipRotation(rootNodes);
    }
    
    void ConvertToRHCProcess::FlipRotation(SharedPtr<Node> node)
    {
        if (!node)
            return;
        
        glm::mat4& matrix = node->LocalTransform;
        for (int i = 0; i < 3; ++i) {
            matrix[i][2] *= -1; // Negate the third column
            matrix[2][i] *= -1; // Negate the third row
        }
        // Flip the Z translation component
        matrix[2][3] *= -1;
        
        for (auto& child : node->Children)
            FlipRotation(child);
    }
}
