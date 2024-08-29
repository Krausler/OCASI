#pragma once

#include "OCASI/Core/Base.h"
#include "glm/glm.hpp"

namespace OCASI {

    constexpr uint32_t INVALID_PARENT_ID = -1;

    struct Mesh 
    {
        std::string Name = "";

        std::vector<glm::vec3> Positions;
        std::vector<glm::vec3> Normals;
        std::vector<glm::vec2> TexCoords;
        std::vector<glm::vec3> Tangents; // Optional

        std::vector<uint32_t> Indices;

        uint32_t Parent = -1;
        bool HasTexCoords;
        bool HasNormals;
        bool HasTangents = false;
    };

}