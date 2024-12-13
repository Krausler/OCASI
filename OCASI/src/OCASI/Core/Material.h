#pragma once

#include "OCASI/Core/Base.h"
#include "OCASI/Core/Image.h"

#include "glm/glm.hpp"

namespace OCASI {

    // The index into the MATERIAL_VALUE_OBJECT_SIZES where the object's size is specified
    const size_t MATERIAL_ALBEDO_COLOUR_INDEX = 0; // Object size: glm::vec3; this may also be the diffuse colour
    const size_t MATERIAL_AMBIENT_COLOUR_INDEX = 1; // Object size: glm::vec3
    const size_t MATERIAL_SPECULAR_COLOUR_INDEX = 2; // Object size: glm::vec3
    const size_t MATERIAL_EMISSIVE_COLOUR_INDEX = 3; // Object size: glm::vec3
    const size_t MATERIAL_ROUGHNESS_INDEX = 4; // Object size: float
    const size_t MATERIAL_METALLIC_INDEX = 5; // Object size: float
    const size_t MATERIAL_ANISOTROPY_INDEX = 6; // Object size: float
    const size_t MATERIAL_ANISOTROPY_ROTATION_INDEX = 7; // Object size: float
    const size_t MATERIAL_CLEARCOAT_INDEX = 8; // Object size: float
    const size_t MATERIAL_CLEARCOAT_ROUGHNESS_INDEX = 9; // Object size: float
    const size_t MATERIAL_SPECULAR_STRENGTH_INDEX = 10; // Object size: float; this is also called shininess
    const size_t MATERIAL_EMISSIVE_STRENGTH_INDEX = 11; // Object size: float
    const size_t MATERIAL_TRANSPARENCY_INDEX = 12; // Object size: float
    const size_t MATERIAL_IOR_INDEX = 13; // Object size: float

    const size_t MATERIAL_BUFFER_BYTE_SIZE = 14;

    const size_t MATERIAL_VALUE_OBJECT_SIZES[] =
    {
            sizeof(glm::vec3), // albedo
            sizeof(glm::vec3), // ambient
            sizeof(glm::vec3), // specular
            sizeof(glm::vec3), // emissive
            sizeof(float), // roughness
            sizeof(float), // metallic
            sizeof(float), // anisotropy
            sizeof(float), // anisotropy rotation
            sizeof(float), // clearcoat
            sizeof(float), // clearcoat roughness
            sizeof(float), // specular strength
            sizeof(float), // emissive strength
            sizeof(float), // transparency
            sizeof(float), // ior (index of refraction)
    };

    // The index at which the texture lies in the m_Texture array from the Material class
    const size_t MATERIAL_TEXTURE_ALBEDO_INDEX = 0;
    const size_t MATERIAL_TEXTURE_AMBIENT_INDEX = 1;
    const size_t MATERIAL_TEXTURE_SPECULAR_INDEX = 2;
    const size_t MATERIAL_TEXTURE_EMISSIVE_INDEX = 3;
    const size_t MATERIAL_TEXTURE_ROUGHNESS_INDEX = 4;
    const size_t MATERIAL_TEXTURE_METALLIC_INDEX = 5;
    const size_t MATERIAL_TEXTURE_CLEARCOAT_INDEX = 6;
    const size_t MATERIAL_TEXTURE_CLEARCOAT_ROUGHNESS_INDEX = 7;
    const size_t MATERIAL_TEXTURE_NORMAL_INDEX = 8;
    const size_t MATERIAL_TEXTURE_EMISSIVE_STRENGTH_INDEX = 9;
    const size_t MATERIAL_TEXTURE_SPECULAR_STRENGTH_INDEX = 10;
    const size_t MATERIAL_TEXTURE_ANISOTROPY_INDEX = 11; // This texture holds both the anisotropy and the rotation
    const size_t MATERIAL_TEXTURE_TRANSPARENCY_INDEX = 12;
    const size_t MATERIAL_TEXTURE_OCCLUSION_INDEX = 13;

    // Reflection maps
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_TOP = 14;
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_BOTTOM = 15;
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_FRONT = 16;
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_BACK = 17;
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_RIGHT = 18;
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_LEFT = 19;
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_SPHERE = 20;

    const size_t MATERIAL_TEXTURE_ARRAY_SIZE = 21;

    class Material
    {
    public:
        Material() = default;

        template<typename Type>
        void SetValue(size_t index, const Type& value);

        template<typename Type>
        Type GetValue(size_t index);

        void SetTexture(size_t index, std::shared_ptr<Image> image);
        std::shared_ptr<Image> GetTexture(size_t index);

        void SetName(const std::string& name);
        void SetCombinedMetallicRoughnessTexture(bool value) { m_UseCombinedMetallicRoughnessTexture = value; }

        bool UsesCombinedMetallicRoughnessTexture() { return m_UseCombinedMetallicRoughnessTexture; }
        const std::string& GetName() { return m_Name; }
    private:
        static consteval size_t GetMaterialValueObjectsSizesArraySize() { return sizeof(MATERIAL_VALUE_OBJECT_SIZES) / sizeof(MATERIAL_VALUE_OBJECT_SIZES[0]); }
        static consteval size_t CalculateMaterialBufferByteSize();

        static constexpr size_t CalculateOffset(size_t index);
    private:
        std::string m_Name;
        bool m_UseCombinedMetallicRoughnessTexture = false;
        std::array<uint8_t, MATERIAL_BUFFER_BYTE_SIZE> m_MaterialValues;
        std::array<std::shared_ptr<Image>, MATERIAL_TEXTURE_ARRAY_SIZE> m_MaterialTextures;
    };

}
