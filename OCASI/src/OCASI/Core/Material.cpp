#include "Material.h"

namespace OCASI {

    template<>
    void Material::SetValue(size_t index, const glm::vec3& value)
    {
        OCASI_ASSERT(index < GetMaterialValueObjectsSizesArraySize());
        size_t offset = CalculateOffset(index);
        OCASI_ASSERT(offset != -1);

        std::memcpy(m_MaterialValues.data() + offset, &value, sizeof(glm::vec3));
    }

    template<>
    void Material::SetValue(size_t index, const float& value)
    {
        OCASI_ASSERT(index < GetMaterialValueObjectsSizesArraySize());
        size_t offset = CalculateOffset(index);
        OCASI_ASSERT(offset != -1);

        std::memcpy(m_MaterialValues.data() + offset, &value, sizeof(float));
    }

    template<>
    glm::vec3 Material::GetValue(size_t index)
    {
        OCASI_ASSERT(index < GetMaterialValueObjectsSizesArraySize());
        size_t offset = CalculateOffset(index);
        OCASI_ASSERT(offset != -1);

        glm::vec3 val;
        std::memcpy(&val, m_MaterialValues.data() + offset, sizeof(glm::vec3));
        return val;
    }

    template<>
    float Material::GetValue(size_t index)
    {
        OCASI_ASSERT(index < GetMaterialValueObjectsSizesArraySize());
        size_t offset = CalculateOffset(index);
        OCASI_ASSERT(offset != -1);

        float val;
        std::memcpy(&val, m_MaterialValues.data() + offset, sizeof(float));
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

    consteval size_t Material::CalculateMaterialBufferByteSize()
    {
        size_t byteSize = 0;
        for (size_t i = 0; i < GetMaterialValueObjectsSizesArraySize(); i++)
        {
            byteSize += MATERIAL_VALUE_OBJECT_SIZES[i];
        }

        return byteSize;
    }

    constexpr size_t Material::CalculateOffset(size_t index)
    {
        if (index >= GetMaterialValueObjectsSizesArraySize())
            return -1;

        size_t offset = 0;
        for (size_t i = 0; i < index; i++)
        {
            offset += MATERIAL_VALUE_OBJECT_SIZES[i];
        }

        return 0;
    }

    void Material::SetName(const std::string& name)
    {
        m_Name = name;
    }
}