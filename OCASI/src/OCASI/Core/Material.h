#pragma once

#include "OCASI/Core/Base.h"
#include "OCASI/Core/Image.h"

#include "glm/glm.hpp"

namespace OCASI {

    struct Material
    {
        // The materials name
        std::string Name;

        // Albedo: Deprecated way of specifying an objects color
        glm::vec3 AlbedoColour;
        std::unique_ptr<Image> AlbedoTexture = nullptr;

        // Diffuse: The colour, that is displayed, when light bounces in all directions
        glm::vec3 DiffuseColour;
        std::unique_ptr<Image> DiffuseTexture = nullptr;

        // Specular: The colour, that is displayed, when specular highlights are seen
        glm::vec3 SpecularColour;
        std::unique_ptr<Image> SpecularTexture = nullptr;

        // Emissive: The colour of an object, if it emits light, acting as a light source
        glm::vec3 EmissiveColour;
        std::unique_ptr<Image> EmissiveTexture = nullptr;

        // Shininess: The value, defining how intense specular highlights are rendered
        float Shininess = 0.0f;
        std::unique_ptr<Image> ShininessTexture = nullptr;

        float Transparency = 1.0f;
        std::unique_ptr<Image> TransparencyTexture = nullptr;

        float BumpMultiplier = 1.0f;
        std::unique_ptr<Image> Bump = nullptr;
        std::unique_ptr<Image> NormalMap = nullptr;
        std::vector<std::unique_ptr<Image>> ReflectionMaps; // The orientation of these texture is specified using ImageOrientation in ImageSettings

        /// PBR

        // Metallic
        float Metallic = 0.0f;
        std::unique_ptr<Image> MetallicTexture = nullptr;

        // Roughness
        float Roughness = 0.0f;
        std::unique_ptr<Image> RoughnessTexture = nullptr;

        float Sheen = 0.0f;
        std::unique_ptr<Image> SheenTexture = nullptr;

        float Clearcoat = 0.0f;
        std::unique_ptr<Image> ClearcoatTexture = nullptr;

        float ClearcoatRoughness = 0.0f;
        std::unique_ptr<Image> ClearcoatRoughnessTexture = nullptr;

        float Anisotropy = 0.0f;
        float AnisotropyRotation = 0.0f;

        std::unique_ptr<Image> AmbientOcclusionTexture = nullptr;
    };

}