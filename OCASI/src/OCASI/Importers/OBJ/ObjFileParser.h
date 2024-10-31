#pragma once

#include "OCASI/Core/FileUtil.h"
#include "OCASI/Importers/OBJ/OBJModel.h"

namespace OCASI::OBJ {

    constexpr size_t INVALID_ID = -1;

    struct Vertex
    {
        size_t VertexIndex = INVALID_ID;
        size_t TextureCoordinateIndex = INVALID_ID;
        size_t NormalIndex = INVALID_ID;

        bool operator==(const Vertex& other) const {
            return VertexIndex == other.VertexIndex &&
                   TextureCoordinateIndex == other.TextureCoordinateIndex &&
                   NormalIndex == other.NormalIndex;
        }
    };

    class ObjFileParser
    {
    public:
        ObjFileParser(FileReader& reader);
        ~ObjFileParser() = default;

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

        glm::vec4 ParseVec4();
        glm::vec3 ParseVec3();
        glm::vec2 ParseVec2();
    private:
        using FileDataIterator = std::vector<char>::iterator;

        FileReader& m_FileReader;
        std::shared_ptr<Model> m_OBJModel;

        FileDataIterator m_Begin, m_End;

        std::vector<std::shared_ptr<Vertex>> m_Vertices;
        std::vector<glm::vec3> m_VertexColours;
        std::vector<glm::vec3> m_Normals;
        std::vector<glm::vec2> m_TexCoords;

        std::unordered_map<uint32_t, std::vector<std::shared_ptr<Vertex>>> m_SortedLookupTable;

        Mesh* m_CurrentMesh = nullptr;
        Object* m_CurrentObject = nullptr;
    };
}

namespace std {
    template <>
    struct hash<OCASI::OBJ::Vertex> {
        std::size_t operator()(const OCASI::OBJ::Vertex& vertex) const {
            std::size_t h1 = std::hash<size_t>{}(vertex.VertexIndex);
            std::size_t h2 = std::hash<size_t>{}(vertex.TextureCoordinateIndex);
            std::size_t h3 = std::hash<size_t>{}(vertex.NormalIndex);

            // Combine the hashes with XOR and bit shifts
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}
