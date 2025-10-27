#pragma once

#include "OCASI/Importers/OBJ/Model.h"
#include "OCBase/IO/FileStream.h"

namespace OCASI::OBJ {

    constexpr size_t INVALID_ID = -1;

    class FileParser
    {
    public:
        FileParser(OCBase::FileStreamReader& reader);
        ~FileParser() = default;

        ExpectedImportT<SharedPtr<Model>> ParseOBJFile();
    private:
        void ParseVertex2D();
        void ParseVertex3D();
        void ParseVertexColour();
        void ParseTextureCoordinate();
        void ParseNormal();
        ExpectedImport ParseFace();

        void ProcessGroup();
        void ProcessObject();
        void ProcessMaterialAssignment();

        size_t CreateObject(const String& name);
        size_t CreateMesh(const String& name);
//        void CreateNewVertex(Vertex& v);

        glm::vec3 ParseVec3();
        glm::vec2 ParseVec2();
    private:
        using FileDataIterator = Vector<char>::iterator;
        
        OCBase::FileStreamReader& m_FileReader;
        SharedPtr<Model> m_OBJModel;

        FileDataIterator m_Begin, m_End;

        Mesh* m_CurrentMesh = nullptr;
        Object* m_CurrentObject = nullptr;
        bool m_GroupActive = false;
    };
}