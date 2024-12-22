#pragma once

#include "OCASI/Core/FileUtil.h"
#include "OCASI/Importers/OBJ/Model.h"

namespace OCASI::OBJ {

    constexpr size_t INVALID_ID = -1;

    class FileParser
    {
    public:
        FileParser(FileReader& reader);
        ~FileParser() = default;

        std::shared_ptr<Model> ParseOBJFile();
    private:
        void ParseVertex2D();
        void ParseVertex3D();
        void ParseVertexColour();
        void ParseTextureCoordinate();
        void ParseNormal();
        void ParseFace();

        void ProcessGroup();
        void ProcessObject();
        void ProcessMaterialAssignment();

        size_t CreateObject(const std::string& name);
        size_t CreateMesh(const std::string& name);
//        void CreateNewVertex(Vertex& v);

        glm::vec3 ParseVec3();
        glm::vec2 ParseVec2();
    private:
        using FileDataIterator = std::vector<char>::iterator;

        FileReader& m_FileReader;
        std::shared_ptr<Model> m_OBJModel;

        FileDataIterator m_Begin, m_End;

        Mesh* m_CurrentMesh = nullptr;
        Object* m_CurrentObject = nullptr;
        bool m_GroupActive = false;
    };
}