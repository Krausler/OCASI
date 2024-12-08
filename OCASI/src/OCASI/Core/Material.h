#pragma once

#include "OCASI/Core/Base.h"
#include "OCASI/Core/Image.h"

#include "glm/glm.hpp"

namespace OCASI {

    // TODO: Rework textures. Create a texture system with an array of texture objects, using an enum for indexing.

    struct PBRMaterial
    {
        glm::vec3 AlbedoColour = glm::vec3(1); // The objects base color;
        std::unique_ptr<Image> AlbedoTexture = nullptr;

        std::unique_ptr<Image> NormalMap = nullptr;

        float Metallic = 0; // Value determining the mix value between the IOR and the objects albedo colour (default: defines that the object does not have a metallic appearance)
        std::unique_ptr<Image> MetallicTexture = nullptr;

        float Roughness = 0.5f; // Value determining the roughness of a surface (default: half of the maximum value)
        std::unique_ptr<Image> RoughnessTexture = nullptr;

        float IOR = 0.04f; // Index of refraction or the base reflectivity of the object (default: base reflectivity of plastic)
        std::unique_ptr<Image> AmbientOcclusionTexture = nullptr;

        // Anisotropy can often be found brushed metal like aluminum
        float Anisotropy = 0.0f; // The amount of additional reflection added to the objects surface
        float AnisotropyRotation = 0.0f; // The rotation of the reflection

        float Clearcoat = 0.0f;
        std::unique_ptr<Image> ClearcoatTexture;
        float ClearcoatRoughness = 0.0f;
        std::unique_ptr<Image> ClearcoatRoughnessTexture;
    };

    // Some values are duplicates as they are used in both models e.g. the normal map
    struct BlinnPhongMaterial
    {
        // Diffuse: The colour that is displayed when light bounces in all directions
        glm::vec3 DiffuseColour;
        std::unique_ptr<Image> DiffuseTexture = nullptr;

        // Diffuse: The colour that is displayed when light bounces in all directions
        glm::vec3 AmbientColour;
        std::unique_ptr<Image> AmbientTexture = nullptr;

        // Specular: The colour that is displayed when specular highlights are computed
        glm::vec3 SpecularColour;
        std::unique_ptr<Image> SpecularTexture = nullptr;

        // Emissive: The colour of an object when it emits light acting as a light source
        glm::vec3 EmissiveColour;
        std::unique_ptr<Image> EmissiveTexture = nullptr;

        // Shininess: The value defining how intense specular highlights are rendered
        float Shininess = 0.0f;
        std::unique_ptr<Image> ShininessTexture = nullptr;

        float Transparency = 1.0f;
        std::unique_ptr<Image> TransparencyTexture = nullptr;
        std::vector<std::unique_ptr<Image>> ReflectionMaps;

        std::unique_ptr<Image> NormalMap = nullptr;
    };

    struct Material
    {
        std::string Name;

        std::unique_ptr<PBRMaterial> PbrMaterial = nullptr;
        std::unique_ptr<BlinnPhongMaterial> BlinnPhongMaterial = nullptr;

        bool IsPbrMaterial() const { return PbrMaterial != nullptr; }
        bool IsBlinnPhongMaterial() const { return BlinnPhongMaterial != nullptr; }
    };

}
