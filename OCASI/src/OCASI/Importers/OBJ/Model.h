#pragma once

#include <utility>

#include "OCASI/Core/Base.h"

#include "OCASI/Core/Image.h"
#include "OCASI/Core/Model.h"

#include "glm/glm.hpp"

namespace OCASI::OBJ {

    const uint8_t MAX_NON_PBR_TEXTURE_COUNT = 15;
    const uint8_t MAX_PBR_TEXTURE_COUNT = 9;
    const uint8_t PBR_TEXTURE_TYPE_ARRAY_NORMALIZER = MAX_NON_PBR_TEXTURE_COUNT;

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
        Normal = 7,

        ReflectionTop = 8,
        ReflectionBottom = 9,
        ReflectionFront = 10,
        ReflectionBack = 11,
        ReflectionLeft = 12,
        ReflectionRight = 13,
        ReflectionSphere = 14,

        // PBR
        Albedo = 15,
        EmissivePBR = 16,
        Roughness = 17,
        Metallic = 18,
        Sheen = 19,
        Clearcoat = 20,
        ClearcoatRoughness = 21,
        Occlusion = 22,
        NormalPBR = 23,

        // Only for specifying, that a reflection map is being parsed. No need to have a texture slot in the array for this texture value.
        Reflection
    };

    bool HasPBRTextureType(TextureType type) { return type >= MAX_NON_PBR_TEXTURE_COUNT && type < MAX_NON_PBR_TEXTURE_COUNT + MAX_PBR_TEXTURE_COUNT; }

    struct PBRMaterialExtension
    {
        glm::vec3 AlbedoColour = glm::vec3(1);
        glm::vec3 EmissiveColour = glm::vec3(0);

        float Roughness = 0.0f;                     // Roughness (Pr)
        float Metallic = 0.0f;                      // Metallic (Pm)

        float Sheen = 0.0f;                         // ExtSheen intensity (Ps)

        float Clearcoat = 0.0f;                     // ExtClearcoat intensity (Pc)
        float ClearcoatRoughness = 0.0f;            // ExtClearcoat roughness (Pcr)

        float Anisotropy = 0.0f;                    // ExtAnisotropy (an / aniso)
        float AnisotropyRotation = 0.0f;            // ExtAnisotropy rotation angle (anR / anisoR)

        // Index of refraction (optical density)
        float IOR = 1.0f;

        std::array<std::string, MAX_PBR_TEXTURE_COUNT> Textures = {}; // Textures (map_{Values}) using TextureType as an index
        std::array<bool, MAX_PBR_TEXTURE_COUNT> TextureClamps = {}; // Specifies whether a texture is clamped
    };

    struct Material {
        std::string Name;

        // Basic material colors
        glm::vec3 AmbientColour = glm::vec3(1);   // Ambient color (Ka)
        glm::vec3 DiffuseColour = glm::vec3(1);   // Diffuse color (Kd)
        glm::vec3 SpecularColour = glm::vec3(0);  // ExtSpecular color (Ks)
        glm::vec3 EmissiveColour = glm::vec3(0);  // Emissive color (Ke)

        // Reflectivity and transparency
        float Shininess = 0.0f;                     // Shininess (Ns)
        float Opacity = 1.0f;                       // Transparency (d or Tr)                        // Index of refraction (Ni)

        std::array<std::string, MAX_NON_PBR_TEXTURE_COUNT> Textures = {}; // Textures (map_{Values}) using TextureType as an index
        std::array<bool, MAX_NON_PBR_TEXTURE_COUNT> TextureClamps = {}; // Specifies whether a texture is clamped
        float BumpMapMultiplier = 1.0f;

        std::optional<PBRMaterialExtension> PBRExtension;
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
        Mesh(std::string name = "Model")
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