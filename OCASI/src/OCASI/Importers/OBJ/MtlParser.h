#pragma once

#include "OCASI/Core/Base.h"
#include "OCASI/Importers/OBJ/Model.h"

#include "OCBase/IO/FileStream.h"

namespace OCASI::OBJ {

    class MtlParser
    {
    public:
        MtlParser(const SharedPtr<Model>& model, OCBase::FileStreamReader& reader);
        ~MtlParser() = default;
        
        ExpectedImport ParseMTLFile();

    private:
        ExpectedImport ParseParameter(bool isMap);
        void CreateNewMaterial(const String& name);
        void ParseTexture(TextureType type);

        float ParseFloat();
        glm::vec3 ParseVec3();
        glm::vec4 ParseVec4();
        
        bool CheckMaterial();
        void CreatePBRMaterialExtension();
    private:
        using FileDataIterator = Vector<char>::iterator;

        OCBase::FileStreamReader& m_Reader;
        FileDataIterator m_Begin, m_End;

        SharedPtr<Model> m_Model = nullptr;
        Material* m_CurrentMaterial = nullptr;
    };
}
