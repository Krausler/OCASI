#pragma once

#include <utility>

#include "OCASI/Core/Base.h"

#include "OCASI/Core/Image.h"
#include "OCASI/Core/Mesh.h"

#include "glm/glm.hpp"

namespace OCASI::OBJ {

    constexpr uint8_t MAX_TEXTURE_COUNT = 21;

    enum TextureType
    {
        None = -1,
        Ambient = 0,
        Diffuse = 1,
        Specular = 2,
        Emissive = 3,
        Transparency = 4,
        Bump = 5,
        Shininess = 6,

        // PBR
        Roughness = 7,
        Metallic = 8,
        Sheen = 9,
        Clearcoat = 10,
        ClearcoatRoughness = 11,
        Occlusion = 12,
        Normal = 13,

        ReflectionTop = 14,
        ReflectionBottom = 15,
        ReflectionFront = 16,
        ReflectionBack = 17,
        ReflectionLeft = 18,
        ReflectionRight = 19,
        ReflectionSphere = 20,

        Reflection // Only for specifying, that a reflection map is being parsed. No need to have a texture slot in the array.
    };

    struct Material {
        std::string Name;

        // Basic material colors
        glm::vec3 AmbientColour = glm::vec3(1);   // Ambient color (Ka)
        glm::vec3 DiffuseColour = glm::vec3(1);   // Diffuse color (Kd)
        glm::vec3 SpecularColour = glm::vec3(0);  // Specular color (Ks)
        glm::vec3 EmissiveColour = glm::vec3(0);  // Emissive color (Ke)

        // Reflectivity and transparency
        float Shininess = 0.0f;                     // Shininess (Ns)
        float Opacity = 1.0f;                       // Transparency (d or Tr)

        // Index of refraction (optical density)
        float IOR = 1.0f;                           // Index of refraction (Ni)

        // Unofficial PBR Extensions
        float Roughness = 0.0f;                     // Roughness (Pr)
        float Metallic = 0.0f;                      // Metallic (Pm)
        float Sheen = 0.0f;                         // Sheen intensity (Ps)
        float Clearcoat = 0.0f;                     // Clearcoat intensity (Pc)
        float ClearcoatRoughness = 0.0f;            // Clearcoat roughness (Pcr)
        float Anisotropy = 0.0f;                    // Anisotropy (an)
        float AnisotropyRotation = 0.0f;            // Anisotropy rotation angle (anR)

        std::array<std::string, MAX_TEXTURE_COUNT> Textures = {}; // Textures (map_{Values}) using TextureType as an index
        std::array<bool, MAX_TEXTURE_COUNT> TextureClamps = {}; // Specifies whether a texture is clamped
        float BumpMapMultiplier = 1.0f;
    };

    struct Face
    {
        std::vector<size_t> VertexIndices;
        std::vector<size_t> TextureCoordinateIndices;
        std::vector<size_t> NormalIndices;

        FaceType Type;
    };

    struct Mesh
    {
        Mesh(std::string name = "Mesh")
            : Name(std::move(name))
        {}

        std::string Name;
        std::string MaterialName;

        std::vector<Face> Faces;

        OCASI::FaceType FaceType = FaceType::None;
        Dimension Dimension = Dimension::None;
    };

    struct Object
    {
        std::string Name;

        uint32_t Mesh;
        std::vector<uint32_t> Children;
    };

    struct Model
    {
        std::string Name;
        std::string MTLFilePath;

        std::unordered_map<std::string, Material> Materials;
        std::vector<Mesh> Meshes;
        std::vector<Object> Objects;

        std::vector<glm::vec3> Vertices;
        std::vector<glm::vec3> VertexColours;
        std::vector<glm::vec3> Normals;
        std::vector<glm::vec2> TexCoords;
    };
}