#pragma once

#include "OCASI/Core/Base.h"
#include "OCASI/Core/FileUtil.h"

#include "OCASI/Importers/OBJ/Model.h"

namespace OCASI::OBJ {

    class MtlParser
    {
    public:
        MtlParser(const std::shared_ptr<Model>& model, const Path& relativePath);
        ~MtlParser() = default;

        bool ParseMTLFile();

    private:
        bool ParseParameter(bool isMap);
        void CreateNewMaterial(const std::string& name);
        void ParseTexture(TextureType type);

        float ParseFloat();
        glm::vec3 ParseVec3();
        glm::vec4 ParseVec4();

        bool CheckMaterial();
        void CreatePBRMaterialExtension();
    private:
        using FileDataIterator = std::vector<char>::iterator;

        FileReader m_Reader;
        FileDataIterator m_Begin, m_End;

        std::shared_ptr<Model> m_Model = nullptr;
        Material* m_CurrentMaterial = nullptr;
    };
}
