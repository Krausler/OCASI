#pragma once

#include "OCASI/Core/Base.h"
#include "glm/glm.hpp"

namespace OCASI {

    struct Mesh 
    {
        std::vector<glm::vec3> Positions;
        std::vector<glm::vec3> Normals;
        std::vector<glm::vec2> TexCoords;
        std::vector<glm::vec3> Tangents; // Optional

        bool HasTangents;
    };

}