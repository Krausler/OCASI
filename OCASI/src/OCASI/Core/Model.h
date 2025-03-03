#pragma once

#include "OCASI/Core/Base.h"

#include "glm/glm.hpp"

#include <array>

namespace OCASI {

    //! @brief The maximum amount of texture coordinate sets a mesh can have.
    const uint8_t TEXTURE_COORDINATE_ARRAY_SIZE = 5;
    const size_t INVALID_ID = -1;

    //! @brief Specifies what primitive type the mesh vertex data is made of.
    enum class FaceType
    {
        None = 0,
        Point = 1,
        //! Straight lines (no bezier curves)
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

    //! @brief Specifies how many components the vertex data's position has.
    enum class Dimension
    {
        None = 0,
        _1D = 1,
        _2D = 2,
        _3D = 3
    };

    /*! @brief A mesh holds vertex data of a consecutive structure represented by positions, normals, texture coordinates, vertex colours,
     *         tangents and indices.
     *
     *  A mesh is required to have vertex positions, indices and at least one set of texture coordinates. Normals, tangents and vertex
     *  colours are optional parameters, that need to be checked for. Additionally meshes may have a material associated with it, if the
     *  MaterialIndex does not equal INVALID_ID. If the index is valid, it may be used as an index into a Scenes Material array.
     */
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
        
        bool HasVertexColours() const { return !VertexColours.empty(); }
        bool HasNormals() const { return !Normals.empty(); }
        bool HasTangents() const { return !Tangents.empty(); }
    };

    /*! @brief A model is a collection of multiple meshes, that belong together. When rendered, models should appear as one single
     *         object, with the individual meshes being able to be moved or modified.
     *
     *  Rendering applications, like game engines, may use individual mesh highlighting for editing of materials and such. It is recommended
     *  to treat a model as an physics entity, and not the meshes individually.
     */
    struct Model
    {
        std::string Name;

        std::vector<Mesh> Meshes;
    };

}