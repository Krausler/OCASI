#include "GenerateNormalsProcess.h"

#include "OCASI/Core/Scene.h"

namespace OCASI {
    bool GenerateNormalsProcess::NeedsProcessing(SharedPtr<Scene> scene, SharedPtr<BaseImporter> importer)
    {
        BasePostProcess::NeedsProcessingDefault(this, scene, importer);
        
        for (size_t i = 0; i < scene->Models.size(); i++)
        {
            for (size_t j = 0; j < m_Scene->Models.at(i).Meshes.size(); j++)
            {
                const Mesh& mesh = m_Scene->Models.at(i).Meshes.at(j);
                
                // When the mesh has no normals it is registered for processing
                if (mesh.Normals.empty())
                    m_ModelsWithProcessingNeed.emplace_back(i, j);
                
                // Impossible to calculate normals for lines or points
                if (mesh.FaceMode & FaceType::Point || mesh.FaceMode & FaceType::Line)
                {
                    OCASI_LOG_INFO("Normal generation of meshes with FaceType, of type line or point, is not supported.");
                    continue;
                }
            }
        }
        
        return !m_ModelsWithProcessingNeed.empty();
    }
    
    void GenerateNormalsProcess::ExecuteProcess()
    {
        for (auto [model, meshIndex] : m_ModelsWithProcessingNeed)
        {
            Mesh& mesh = m_Scene->Models.at(model).Meshes.at(meshIndex);
            
            size_t verticesPerFace = (size_t) mesh.FaceMode;
            OCASI_ASSERT(verticesPerFace >= 3 && verticesPerFace <= 4);
            
            // Stores the normal per vertex and all its surrounding neighbors normals added to it,
            // with the number of adjacent faces to that vertex
            std::vector<std::pair<glm::vec3, size_t>> vertexNormals(mesh.Vertices.size(), { glm::vec3(0), 0 });
            for (size_t i = 0; i < mesh.Indices.size(); i += verticesPerFace)
            {
                // Calculates the normal of a face, by taking the cross-product of two face edges,
                // with the same origin. The cross-product returns a vector perpendicular to both
                // of the input vectors.
                glm::vec3 edge1 = mesh.Vertices.at(mesh.Indices.at(i + 1)) - mesh.Vertices.at(mesh.Indices.at(i + 0));
                glm::vec3 edge2 = mesh.Vertices.at(mesh.Indices.at(i + 2)) - mesh.Vertices.at(mesh.Indices.at(i + 0));
                glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
                
                // For every vertex, the normal gets added on top and the total count of added normals
                // increased by 1.
                for (size_t j = 0; j < verticesPerFace; j++)
                {
                    auto& [n, count] = vertexNormals[mesh.Indices.at(i + j)];
                    n += normal;
                    ++count;
                }
            }
            
            mesh.Normals.resize(mesh.Vertices.size());
            for (size_t i = 0; i < mesh.Vertices.size(); i++)
            {
                auto& [normals, count] = vertexNormals.at(i);
                mesh.Normals[i] = glm::normalize(normals / (float)count);
            }
        }
    }
}