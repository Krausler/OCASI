#pragma once

#include "OCASI/Core/Base.h"
#include "OCASI/Core/Image.h"

#include "glm/glm.hpp"

namespace OCASI {

    // The index into the MATERIAL_VALUE_OBJECT_SIZES where the object's size is specified
    const size_t MATERIAL_ALBEDO_COLOUR = 0; // Object size: glm::vec4
    const size_t MATERIAL_DIFFUSE_COLOUR = MATERIAL_ALBEDO_COLOUR; // Object size: glm::vec4
    const size_t MATERIAL_AMBIENT_COLOUR = 1; // Object size: glm::vec4
    const size_t MATERIAL_SPECULAR_COLOUR = 2; // Object size: glm::vec4
    const size_t MATERIAL_EMISSIVE_COLOUR = 3; // Object size: glm::vec4
    const size_t MATERIAL_ROUGHNESS = 4; // Object size: float
    const size_t MATERIAL_METALLIC = 5; // Object size: float
    const size_t MATERIAL_ANISOTROPY = 6; // Object size: float
    const size_t MATERIAL_ANISOTROPY_ROTATION = 7; // Object size: float
    const size_t MATERIAL_CLEARCOAT = 8; // Object size: float
    const size_t MATERIAL_CLEARCOAT_ROUGHNESS = 9; // Object size: float
    const size_t MATERIAL_SPECULAR_STRENGTH = 10; // Object size: float; this is also called shininess
    const size_t MATERIAL_EMISSIVE_STRENGTH = 11; // Object size: float
    const size_t MATERIAL_TRANSPARENCY = 12; // Object size: float
    const size_t MATERIAL_IOR = 13; // Object size: float
    const size_t MATERIAL_USE_COMBINED_METALLIC_ROUGHNESS_TEXTURE = 14; // (R, G) = anisotropy rotation in range [-1; 1]; (B) = anisotropy value/strength
    const size_t MATERIAL_USE_COMBINED_ANISOTROPY_ANISOTROPY_ROTATION_TEXTURE = 15; // (G) = roughness; (B) = metallic

    constexpr size_t MATERIAL_VALUE_OBJECT_SIZES[] =
    {
            sizeof(glm::vec4), // albedo
            sizeof(glm::vec4), // ambient
            sizeof(glm::vec4), // specular
            sizeof(glm::vec4), // emissive
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
            sizeof(bool), // combined metallic roughness
            sizeof(bool) // combined anisotropy
    };

    constexpr size_t GetMaterialValueObjectSizesArraySize() { return sizeof(MATERIAL_VALUE_OBJECT_SIZES) / sizeof(MATERIAL_VALUE_OBJECT_SIZES[0]); }
    
    constexpr size_t CalculateMaterialBufferByteSize()
    {
        size_t byteSize = 0;
        for (size_t i = 0; i < GetMaterialValueObjectSizesArraySize(); i++)
        {
            byteSize += MATERIAL_VALUE_OBJECT_SIZES[i];
        }

        return byteSize;
    }

    const size_t MATERIAL_BUFFER_BYTE_SIZE = CalculateMaterialBufferByteSize();

    // The index at which the texture lies in the m_Texture array from the Material class
    const size_t MATERIAL_TEXTURE_ALBEDO = 0;
    const size_t MATERIAL_TEXTURE_DIFFUSE = 0;
    const size_t MATERIAL_TEXTURE_AMBIENT = 1;
    const size_t MATERIAL_TEXTURE_SPECULAR = 2;
    const size_t MATERIAL_TEXTURE_EMISSIVE = 3;
    const size_t MATERIAL_TEXTURE_ROUGHNESS = 4;
    const size_t MATERIAL_TEXTURE_METALLIC = 5;
    const size_t MATERIAL_TEXTURE_CLEARCOAT = 6;
    const size_t MATERIAL_TEXTURE_CLEARCOAT_ROUGHNESS = 7;
    const size_t MATERIAL_TEXTURE_CLEARCOAT_NORMAL = 8;
    const size_t MATERIAL_TEXTURE_NORMAL = 9;
    const size_t MATERIAL_TEXTURE_EMISSIVE_STRENGTH = 10;
    const size_t MATERIAL_TEXTURE_SPECULAR_STRENGTH = 11;
    const size_t MATERIAL_TEXTURE_ANISOTROPY = 12; // This texture can hold both the anisotropy and the rotation, when MATERIAL_USE_COMBINED_ANISOTROPY_ANISOTROPY_ROTATION_TEXTURE is set to true
    const size_t MATERIAL_TEXTURE_ANISOTROPY_ROTATION = 13;
    const size_t MATERIAL_TEXTURE_TRANSPARENCY = 14;
    const size_t MATERIAL_TEXTURE_OCCLUSION = 15;

    // Reflection maps
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_TOP = 16;
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_BOTTOM = 17;
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_FRONT = 18;
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_BACK = 19;
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_RIGHT = 20;
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_LEFT = 21;
    const size_t MATERIAL_TEXTURE_REFLECTION_MAP_SPHERE = 22;

    const size_t MATERIAL_TEXTURE_ARRAY_SIZE = 23;

    class Material
    {
    public:
        Material();

        template<typename Type>
        void SetValue(size_t index, const Type& value);

        template<typename Type>
        Type GetValue(size_t index);

        void* Get(size_t index);
        void Set(uint8_t* value, size_t index, size_t size);

        void SetTexture(size_t index, std::shared_ptr<Image> image);
        std::shared_ptr<Image> GetTexture(size_t index);
        bool HasTexture(size_t index);

        void SetName(const std::string& name);
        const std::string& GetName() const { return m_Name; }
    private:
        static constexpr size_t CalculateOffset(size_t index);
    private:
        std::string m_Name;

        std::array<uint8_t, MATERIAL_BUFFER_BYTE_SIZE> m_MaterialValues;
        std::array<std::shared_ptr<Image>, MATERIAL_TEXTURE_ARRAY_SIZE> m_MaterialTextures;
    };

}
