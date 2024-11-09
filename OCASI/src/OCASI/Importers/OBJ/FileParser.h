#pragma once

#include "OCASI/Core/FileUtil.h"
#include "OCASI/Importers/OBJ/Model.h"

namespace OCASI::OBJ {

    constexpr size_t INVALID_ID = -1;

    struct Vertex
    {
        size_t VertexIndex = INVALID_ID;
        size_t TextureCoordinateIndex = INVALID_ID;
        size_t NormalIndex = INVALID_ID;
        // The index in the indices array where the vertex is referenced
        size_t IndicesIndex = INVALID_ID;

        bool operator==(const Vertex& other) const {
            return VertexIndex == other.VertexIndex && TextureCoordinateIndex == other.TextureCoordinateIndex && NormalIndex == other.NormalIndex;
        }
    };

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
        void CreateMesh(const std::string& name);
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
    };
}