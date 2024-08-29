#include "ObjImporter.h"

#include "OCASI/Core/StringUtil.h"

#include <fstream>

namespace OCASI {

    constexpr uint32_t INVALID_INDEX = 0;

    std::shared_ptr<Scene> ObjImporter::Load3DFile(const Path &path)
    {
        std::shared_ptr<Scene> scene = std::make_unique<Scene>();
        std::ifstream stream(path.string());

        Mesh& currentMesh = scene->Meshes.emplace_back();
        currentMesh.HasTangents = false;
        bool first = true;

        // r stands for "read"
        std::vector<glm::vec3> rPositions;
        std::vector<glm::vec3> rNormals;
        std::vector<glm::vec2> rTexCoords;

        std::unordered_map<uint32_t, uint32_t> IndicesForFinalVertexArray;

        std::string line;
        while (stream)
        {
            std::getline(stream, line);
            
            if (line.empty())
                continue;

            if (line[0] == 'o')
            {
                ObjectGroupingInfo& o = scene->ObjectGroupingInfo.emplace_back();
                o.Name = ReadName(line);
            }

            if (line[0] == 'g')
            {
                if(!scene->ObjectGroupingInfo.empty())
                    scene->ObjectGroupingInfo.at(scene->ObjectGroupingInfo.size() - 1).MeshIndices.push_back(scene->Meshes.size() - 1);

                if(first)
                {
                    first = false;
                    currentMesh.Name = ReadName(line);
                    continue;
                }

                currentMesh = scene->Meshes.emplace_back();
                currentMesh.Name = ReadName(line);
                currentMesh.HasTangents = false;
            }

            // Reading vertex positions
            if (line[0] == 'v')
            {
                rPositions.push_back(ParseVec3(line));
            }

            // Reading vertex normals
            if (line[0] == 'vn')
            {
                rNormals.push_back(ParseVec3(line));
            }

            // Reading texture coordinates 
            if (line[0] == 'vt')
            {
                rTexCoords.push_back(ParseVec2(line));
            }

            // Constructing the vertex (v. position, v. normal, texture coordinate) and the indices 
            if (line[0] == 'f')
            {
                auto vertices = Util::Split(line.substr(2), ' ');
                OCASI_ASSERT_MSG(vertices.size() == 3, "Currently only triangles are supported for faces.");
                for (uint32_t i = 0; i < vertices.size(); i++)
                {
                    uint32_t tokenCount = 0;
                    // Making sure there are no more then three vertices per face
                    auto values = Util::Split(vertices.at(i), '/', tokenCount);
                    OCASI_ASSERT_MSG(vertices.size() > 0 && vertices.size() <= 3 , "Invalid amount of vertex arguments.");
                    
                    uint32_t vertexIndex = std::atof(values[0].c_str());
                    uint32_t index = IndicesForFinalVertexArray.at(vertexIndex); 
                    if (index != INVALID_INDEX)
                    {
                        currentMesh.Indices.push_back(index);
                        continue;
                    }
                    else
                    {
                        IndicesForFinalVertexArray.insert({vertexIndex, currentMesh.Positions.size()});
                        currentMesh.Indices.push_back(currentMesh.Positions.size());
                    }

                    // The OBJ formats first index starts at 1
                    uint32_t vertexIndexToArrayIndex = vertexIndex - 1;
                    switch (values.size())
                    {
                        case 1:
                        {
                            currentMesh.Positions.push_back(rPositions.at(vertexIndex));

                            currentMesh.HasNormals = false;
                            currentMesh.HasTexCoords = false;
                            break;
                        }
                        case 2:
                        {
                            if (tokenCount == 1)
                            {
                                currentMesh.Positions.push_back(rPositions.at(vertexIndex));
                                currentMesh.TexCoords.push_back(rTexCoords.at(std::atof(values.at(1).c_str())));

                                currentMesh.HasTexCoords = true;
                                currentMesh.HasNormals = false;
                            }
                            else if(tokenCount == 2)
                            {
                                currentMesh.Positions.push_back(rPositions.at(vertexIndex));
                                currentMesh.Normals.push_back(rNormals.at(std::atof(values.at(1).c_str())));

                                currentMesh.HasTexCoords = false;
                                currentMesh.HasNormals = true;
                            }
                            break;
                        }
                        case 3:
                        {
                            currentMesh.Positions.push_back(rPositions.at(vertexIndex));
                            currentMesh.TexCoords.push_back(rTexCoords.at(std::atof(values.at(1).c_str())));
                            currentMesh.Normals.push_back(rNormals.at(std::atof(values.at(2).c_str())));

                            currentMesh.HasNormals = true;
                            currentMesh.HasTexCoords = true;
                            break;
                        }
                        default: 
                        {
                            OCASI_LOG_ERROR("Invalid amount of vertex arguments. Only 3 are allowed after f. (e. g. f 25/34/1 ... is valid; f 25/32/12/11 not valid)");
                            return nullptr;
                        }
                    }
                }
            }
        }
        return scene;
    }

    std::string ObjImporter::ReadName(const std::string &line)
    {
        return line.substr(1);
    }

    glm::vec3 ObjImporter::ParseVec3(const std::string &line)
    {
        auto segments = Util::Split(line, ' ');
        return glm::vec3(std::stof(segments[1]), std::stof(segments[2]), std::stof(segments[3]));
    }

    glm::vec2 ObjImporter::ParseVec2(const std::string &line)
    {
        auto segments = Util::Split(line, ' ');
        return glm::vec2(std::stof(segments[1]), std::stof(segments[2]));
    }
}