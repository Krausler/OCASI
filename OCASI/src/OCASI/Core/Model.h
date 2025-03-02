#pragma once

#include "OCASI/Core/Base.h"

#include "glm/glm.hpp"

#include <array>

namespace OCASI {

    const uint8_t TEXTURE_COORDINATE_ARRAY_SIZE = 5;
    const size_t INVALID_ID = -1;

    enum class FaceType
    {
        None = 0,
        Point = 1,
        Line = 2,
        Triangle = 3,
        Quad = 4
    };

    inline FaceType operator|(const FaceType& t, const FaceType& t2)
    {
        return static_cast<FaceType>(static_cast<uint32_t>(t) | static_cast<uint32_t>(t2));
    }

    inline bool operator&(const FaceType& t, const FaceType& t2)
    {
        return static_cast<uint32_t>(t) & static_cast<uint32_t>(t2);
    }

    enum class Dimension
    {
        None = 0,
        _1D = 1,
        _2D = 2,
        _3D = 3
    };

    struct Mesh
    {
        std::string Name;

        std::vector<glm::vec3> Vertices;
        std::vector<glm::vec3> VertexColours;
        std::vector<glm::vec3> Normals;
        std::array<std::vector<glm::vec2>, TEXTURE_COORDINATE_ARRAY_SIZE> TexCoords;
        std::vector<glm::vec4> Tangents; // Optional
        std::vector<uint32_t> Indices;

        size_t MaterialIndex = INVALID_ID;

        FaceType FaceMode = FaceType::None;
        Dimension Dim = Dimension::None;
    };

    struct Model
    {
        std::string Name;

        std::vector<Mesh> Meshes;
    };

}