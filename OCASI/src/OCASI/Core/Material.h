#pragma once

#include "OCASI/Core/Base.h"
#include "OCASI/Core/Image.h"

#include "glm/glm.hpp"

namespace OCASI {

    struct Material
    {
        // Albedo
        glm::vec3 AlbedoColor;
        bool HasTexture = false;
        Image Texture;

        // Roughness
        double Roughness;
        bool HasRoughnessMap = false;
        Image RoughnessMap;

        // Metallic
        double Metallic;
    };

}