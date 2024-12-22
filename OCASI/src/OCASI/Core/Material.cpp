#include "Material.h"

namespace OCASI {

    // TODO: Add a type check for verifying that a requested value has the type of the GetValue function

    template<>
    void Material::SetValue(size_t index, const glm::vec3& value)
    {
        Set(((uint8_t*) &value), index, sizeof(glm::vec3));
    }

    template<>
    void Material::SetValue(size_t index, const glm::vec4& value)
    {
        Set(((uint8_t*) &value), index, sizeof(glm::vec4));
    }

    template<>
    void Material::SetValue(size_t index, const float& value)
    {
        Set((uint8_t*) &value, index, sizeof(float));
    }

    template<>
    void Material::SetValue(size_t index, const bool& value)
    {
        Set((uint8_t*) &value, index, sizeof(bool));
    }

    template<>
    glm::vec3 Material::GetValue(size_t index)
    {
        glm::vec3 val;
        std::memcpy(&val, Get(index), sizeof(glm::vec3));
        return val;
    }

    template<>
    glm::vec4 Material::GetValue(size_t index)
    {
        glm::vec4 val;
        std::memcpy(&val, Get(index), sizeof(glm::vec4));
        return val;
    }

    template<>
    float Material::GetValue(size_t index)
    {
        float val;
        std::memcpy(&val, Get(index), sizeof(float));
        return val;
    }

    template<>
    bool Material::GetValue(size_t index)
    {
        bool val;
        std::memcpy(&val, Get(index), sizeof(bool));
        return val;
    }

    void Material::SetTexture(size_t index, std::shared_ptr<Image> image)
    {
        OCASI_ASSERT(index < MATERIAL_TEXTURE_ARRAY_SIZE);
        m_MaterialTextures[index] = image;
    }

    std::shared_ptr<Image> Material::GetTexture(size_t index)
    {
        OCASI_ASSERT(index < MATERIAL_TEXTURE_ARRAY_SIZE);
        return m_MaterialTextures.at(index);
    }

    void Material::SetName(const std::string& name)
    {
        m_Name = name;
    }

    void* Material::Get(size_t index)
    {
        OCASI_ASSERT(index < GetMaterialValueObjectSizesArraySize());
        size_t offset = CalculateOffset(index);
        OCASI_ASSERT(offset != -1);

        return &m_MaterialValues.at(offset);
    }

    void Material::Set(uint8_t* value, size_t index, size_t size)
    {
        OCASI_ASSERT(index < GetMaterialValueObjectSizesArraySize());
        size_t offset = CalculateOffset(index);
        OCASI_ASSERT(offset != -1);

        std::memcpy(m_MaterialValues.data() + offset, value, size);
    }

    constexpr size_t Material::CalculateOffset(size_t index)
    {
        if (index >= GetMaterialValueObjectSizesArraySize())
            return -1;

        size_t offset = 0;
        for (size_t i = 0; i < index; i++)
        {
            offset += MATERIAL_VALUE_OBJECT_SIZES[i];
        }

        return 0;
    }
}