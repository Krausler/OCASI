#pragma once

#include <utility>

#include "OCASI/Core/Base.h"

#include "OCASI/Core/Image.h"
#include "OCASI/Core/Mesh.h"

#include "glm/glm.hpp"

namespace OCASI::OBJ {
    struct Material
    {
        std::string Name;
        /// @brief Base colour of the material
        glm::vec3 AmbientColour; // TODO: Create a Colour class
        /// @brief Colour of the materials surface when light shines on it
        glm::vec3 DiffuseColour;
        /// @brief Colour of the materials surface when light reflects of it
        glm::vec3 SpecularColour;
        /// @brief Intensity of the specular effect
        float Shininess; // Also specular highlight
        /// @brief Factor of transparency of the surface
        float Transparency;
        /// @brief Colour of transparency
        glm::vec3 TransparencyColour; // Is only used when Transparency is not 0
        /// @brief ??????????????
        float OpticalDensity;

        enum class IllumValue{
            ColorOnAmbientOff = 0,
            ColorOnAmbientOn = 1,
            HighlightOn = 2
        };
        IllumValue Illum;

        /// Extended spec for PBR


        /// @brief Roughness value
        float Roughness;
        /// @brief Metallic value
        float Metallic;
        /// @brief Sheen value
        glm::vec3 Sheen;
        /// @brief Clearcoat thickness value
        float ClearcoatThickness;
        /// @brief Clearcoat roughness value
        float ClearcoatRoughness;
        /// @brief Emissive value
        glm::vec3 Emissive;
        /// @brief Anisotropy value
        float Anisotropy;


        /// Textures
        std::shared_ptr<Image> AmbientTexture = nullptr;
        std::shared_ptr<Image> DiffuseTexture = nullptr;
        std::shared_ptr<Image> SpecularTexture = nullptr;
        std::shared_ptr<Image> ShininessTexture = nullptr;
        std::shared_ptr<Image> EmissiveTexture = nullptr;
        std::shared_ptr<Image> Bump = nullptr;
        std::shared_ptr<Image> NormalMap = nullptr;
        std::shared_ptr<Image> RoughnessTexture = nullptr;
        std::shared_ptr<Image> MetallicTexture = nullptr;
        std::shared_ptr<Image> SheenTexture = nullptr;
    };

    struct Mesh
    {
        Mesh(const std::string& name = "Mesh")
            : Name(name)
        {}

        std::string Name;
        std::string MaterialName;

        std::vector<glm::vec3> Vertices;
        std::vector<glm::vec3> VertexColours;
        std::vector<glm::vec3> Normals;
        std::vector<glm::vec2> TexCoords;

        std::vector<uint32_t> Indices;
        OCASI::FaceType FaceType;
        Dimension Dimension;
    };

    struct Object
    {
        std::string Name;

        uint32_t ParentMesh;
        std::vector<uint32_t> Children;
    };

    struct Model
    {
        std::string Name;
        std::vector<Mesh> Meshes;
        std::unordered_map<std::string, Material> Materials;
        std::vector<Object> Objects;
    };
}