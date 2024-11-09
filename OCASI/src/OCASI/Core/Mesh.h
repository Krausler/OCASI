#pragma once

#include "OCASI/Core/Base.h"
#include "glm/glm.hpp"

namespace OCASI {

    const uint8_t TEXTURE_COORDINATE_ARRAY_SIZE = 5;

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
        std::string Name = "";

        std::vector<glm::vec3> Vertices;
        std::vector<glm::vec3> Normals;
        std::array<std::vector<glm::vec2>, TEXTURE_COORDINATE_ARRAY_SIZE> TexCoords;
        std::vector<glm::vec3> Tangents; // Optional

        std::vector<uint32_t> Indices;
        FaceType FaceType;
        Dimension Dimension;

        bool HasTexCoords;
        bool HasNormals;
        bool HasTangents = false;
    };

}