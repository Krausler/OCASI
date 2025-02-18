#include "TriangulateProcess.h"

#include "OCASI/Core/Scene.h"

namespace OCASI {
    
    bool TriangulateProcess::NeedsProcessing(SharedPtr<Scene> scene, SharedPtr<BaseImporter> importer)
    {
        BasePostProcess::NeedsProcessingDefault(this, scene, importer);
        
        // Selecting the meshes that require triangulation.
        for (size_t i = 0; i < scene->Models.size(); i++)
        {
            for (size_t j = 0; j < m_Scene->Models.at(i).Meshes.size(); j++)
            {
                const Mesh& mesh = m_Scene->Models.at(i).Meshes.at(j);
                
                // Triangulation of lines and points is not supported
                if (mesh.FaceMode == FaceType::Line || mesh.FaceMode == FaceType::Point)
                {
                    OCASI_LOG_INFO("Triangulation of meshes with FaceType, of type line or point, is not supported.");
                    continue;
                }
                
                // Only if the meshes FaceMode exclusively matches Quad it
                // is suitable for triangulation.
                if (mesh.FaceMode == FaceType::Quad)
                    m_ModelsWithProcessingNeed.emplace_back( i, j );
            }
        }
        
        return !m_ModelsWithProcessingNeed.empty();
    }
    
    void TriangulateProcess::ExecuteProcess()
    {
        for (auto [model, meshIndex] : m_ModelsWithProcessingNeed)
        {
            Mesh& mesh = m_Scene->Models.at(model).Meshes.at(meshIndex);
            
            auto oldIndices = mesh.Indices;
            auto& newIndices = mesh.Indices;
            // A quad has 4 vertices. To triangulate a quad, 2 triangles are needed,
            // resulting in 6 vertices for each quad. (4 * 1.5 = 6)
            newIndices.clear();
            newIndices.reserve((size_t)((float)oldIndices.size() * 1.5));
            for (size_t i = 0; i < oldIndices.size(); i += 4)
            {
                // The first triangle is always the combination of the first
                // 3 indices, in order as specified in oldIndices.
                newIndices.push_back(oldIndices.at(i + 0));
                newIndices.push_back(oldIndices.at(i + 1));
                newIndices.push_back(oldIndices.at(i + 2));
                
                // The second triangle is always the first index and the last
                // index of the quad, combined with the remaining, until now,
                // unused vertex.
                newIndices.push_back(oldIndices.at(i + 0));
                newIndices.push_back(oldIndices.at(i + 2));
                newIndices.push_back(oldIndices.at(i + 3));
            }
            
            mesh.FaceMode == FaceType::Triangle;
        }
    }
}