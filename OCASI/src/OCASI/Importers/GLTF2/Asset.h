#pragma once

#include "OCASI/Core/Base.h"

#include "OCASI/Core/FileUtil.h"

#include "glm/glm.hpp"
#include "glm/ext/quaternion_float.hpp"

#include <optional>

namespace OCASI {
    class GLTFImporter;
}

namespace OCASI::GLTF {

    const size_t INVALID_ID = -1;
    const float INVALID_ID_FLOAT = -1;
    const size_t MIN_MAX_ARRAY_SIZE = 16;

    const std::vector<std::string> SUPPORTED_EXTENSIONS =
    {
                "KHR_materials_anisotropy",
                "KHR_materials_clearcoat",
                "KHR_materials_dispersion",
                "KHR_materials_emissive_strength",
                "KHR_materials_ior",
                "KHR_materials_iridescence",
                "KHR_materials_sheen",
                "KHR_materials_specular",
                "KHR_materials_transmission",
                "KHR_materials_unlit",
                "KHR_materials_variants",
                "KHR_materials_volume"
    };

    // The values specify the glTF enum values as stated in the glTF 2.0 spec
    enum class ComponentType
    {
        None = 0,
        Byte = 5120,
        UnsignedByte = 5121,
        Short = 5122,
        UnsignedShort = 5123,
        UnsignedInt = 5124,
        Float = 5126
    };

    size_t ComponentTypeToBytes(ComponentType type);

    constexpr size_t ByteSize(ComponentType type);

    // The values specify the number of components
    enum class DataType
    {
        None = 0,
        Scalar = 1,
        Vec2 = 2,
        Vec3 = 3,
        Vec4 = 4,
        Mat2 = 4,
        Mat3 = 9,
        Mat4 = 16
    };

    enum class PrimitiveType
    {
        Point = 0,
        Line = 1,
        LineLoop = 2,
        LineStrip = 3,
        Triangle = 4,
        TriangleStrip = 5,
        TriangleFan = 6
    };

    enum class AlphaMode
    {
        None = 0,
        Opaque,
        Mask,
        Blend
    };

    enum class MinMagFilter
    {
        None = 0,
        Nearest = 9728,
        Linear = 9729,
        NearestMipMapNearest = 9984,
        NearestMipMapLinear = 9986,
        LinearMipMapNearest = 9985,
        LinearMipMapLinear = 9987
    };

    enum class UVWrap
    {
        Repeat = 10497,
        MirroredRepeat = 33648,
        ClampToEdge = 33071
    };

    class Object
    {
    public:
        Object(uint32_t index)
            : m_Index(index)
        {}
        virtual ~Object() = default;

        size_t GetIndex() const { return m_Index; }

    private:
        size_t m_Index;
    };

    struct BufferView : public Object
    {
        BufferView(size_t index)
            : Object(index)
        {}

        size_t Buffer = INVALID_ID;
        size_t ByteLength = INVALID_ID;
        size_t ByteOffset = 0;
        size_t ByteStride = 0; // Although byteStride does not have a default value, it is easier than making it an optional. It's adding a 0 in an equation.
        // The bufferView.target property is not implemented as it's useless
    };

    struct Sparse
    {
        struct Indices
        {
            size_t BufferView = INVALID_ID;
            size_t ByteOffset = 0;
            ComponentType CompType = ComponentType::None;
        };
        
        struct Values
        {
            size_t BufferView = INVALID_ID;
            size_t ByteOffset = 0;
        };
        
        size_t ElementCount = INVALID_ID;
        Indices Indices;
        Values Values;
    };

    struct Accessor : public Object
    {
        Accessor(size_t index)
            : Object(index)
        {}

        size_t BufferView = INVALID_ID;
        size_t ElementCount = INVALID_ID;
        size_t ByteOffset = 0;
        ComponentType CompType = ComponentType::None;
        bool Normalized = false;
        DataType Type = DataType::None;
        std::array<double, MIN_MAX_ARRAY_SIZE> MinValues;
        std::array<double, MIN_MAX_ARRAY_SIZE> MaxValues;
        std::optional<Sparse> SparseAccessor;
    };

    using VertexAttributes = std::unordered_map<std::string, size_t>;

    // The primitive structs holds indices into the bufferView array
    struct Primitive : public Object
    {
        Primitive(size_t index)
            : Object(index)
        {}

        PrimitiveType Type = PrimitiveType::Triangle;
        // Stores the accessors index using the string key in JSON
        VertexAttributes Attributes;
        size_t MaterialIndex;
        size_t Indices; // The accessor corresponding to the indices
        std::vector<VertexAttributes> MorphTargets;
    };

    struct Mesh : public Object
    {
        Mesh(size_t index)
            : Object(index)
        {}

        std::vector<Primitive> Primitives;
        std::vector<float> Weights; // Morph target weights
    };

    struct Image : public Object
    {
        Image(size_t index)
            : Object(index)
        {}

        std::string URI;

        size_t BufferView = INVALID_ID;
        std::string MimeType;
    };

    struct Sampler : public Object
    {
        Sampler(size_t index)
            : Object(index)
        {}

        MinMagFilter MagFilter = MinMagFilter::None;
        MinMagFilter MinFilter = MinMagFilter::None;
        UVWrap WrapS = UVWrap::Repeat; // Default specified in glTF 2.0 Spec
        UVWrap WrapT = UVWrap::Repeat; // Default specified in glTF 2.0 Spec
    };

    struct Texture : public Object
    {
        Texture(size_t index)
            : Object(index)
        {}

        size_t Sampler = INVALID_ID;
        size_t Source = INVALID_ID;
    };

    struct TextureInfo
    {
        size_t Texture = INVALID_ID;
        size_t TexCoords = 0; // SetValue of texture coordinates
        float Scale = 1.0f;
    };

    struct KHRMaterialPbrSpecularGlossiness
    {
        glm::vec4 DiffuseFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        std::optional<TextureInfo> DiffuseTexture;
        glm::vec3 SpecularFactor = glm::vec3(1.0f, 1.0f, 1.0f);
        std::optional<TextureInfo> SpecularGlossinessTexture;
        float GlossinessFactor = 1.0f;
    };

    struct KHRMaterialClearcoat
    {
        float ClearcoatFactor = 0.0f;
        float ClearcoatRoughnessFactor = 0.0f;
        std::optional<TextureInfo> ClearcoatTexture;
        std::optional<TextureInfo> ClearcoatRoughnessTexture;
        std::optional<TextureInfo> ClearcoatNormalTexture;
    };

    struct KHRMaterialSheen
    {
        std::optional<TextureInfo> SheenColourTexture;
        std::optional<TextureInfo> SheenRoughnessTexture;
        glm::vec3 SheenColourFactor = glm::vec3(0.0f, 0.0f, 0.0f);
        float SheenRoughnessFactor = 0.0f;
    };

    struct KHRMaterialTransmission
    {
        float TransmissionFactor = 0.0f;
        std::optional<TextureInfo> TransmissionTexture;
    };

    struct KHRMaterialVolume
    {
        float ThicknessFactor = 0.0f;
        std::optional<TextureInfo> ThicknessTexture;
        float AttenuationDistance = 0.0f;
        glm::vec3 AttenuationColour = glm::vec3(1.0f, 1.0f, 1.0f);
    };

    struct KHRMaterialIOR
    {
        float IOR = INVALID_ID_FLOAT;
    };

    struct KHRMaterialSpecular
    {
        float SpecularFactor = 1.0f;
        std::optional<TextureInfo> SpecularTexture;
        glm::vec3 SpecularColourFactor = glm::vec3(1.0f, 1.0f, 1.0f);
        std::optional<TextureInfo> SpecularColourTexture;
    };

    struct KHRMaterialEmissiveStrength
    {
        float EmissiveStrength = INVALID_ID_FLOAT;
    };

    struct KHRMaterialIridescence
    {
        float IridescenceFactor = 0.0f;
        std::optional<TextureInfo> IridescenceTexture;
        float IridescenceIor = 1.3f;
        float IridescenceThicknessMinimum = 100.0f;
        float IridescenceThicknessMaximum = 400.0f;
        std::optional<TextureInfo> IridescenceThicknessTexture;
    };

    struct KHRMaterialAnisotropy
    {
        float AnisotropyFactor = 0.0f;
        std::optional<TextureInfo> AnisotropyTexture;
        glm::vec3 AnisotropyDirection = glm::vec3(1.0f, 0.0f, 0.0f);
    };

    struct PBRMetallicRoughness
    {
        glm::vec4 BaseColour = glm::vec4(1);
        std::optional<TextureInfo> BaseColourTexture;

        float Metallic = 1;
        float Roughness = 1;

        std::optional<TextureInfo> MetallicRoughnessTexture;
    };

    struct Material : public Object
    {
        Material(size_t index)
            : Object(index)
        {}

        std::string Name;
        std::optional<PBRMetallicRoughness> MetallicRoughness;

        bool IsDoubleSided = false;
        std::optional<TextureInfo> NormalTexture;
        std::optional<TextureInfo> OcclusionTexture;
        glm::vec3 EmissiveColour = glm::vec3(0);
        std::optional<TextureInfo> EmissiveTexture;

        AlphaMode AMode = AlphaMode::Opaque;
        float AlphaCutoff = 0.5f;

        /// Extensions

        std::optional<KHRMaterialSpecular> ExtSpecular;
        std::optional<KHRMaterialPbrSpecularGlossiness> ExtSpecularGlossiness;
        std::optional<KHRMaterialClearcoat> ExtClearcoat;
        std::optional<KHRMaterialAnisotropy> ExtAnisotropy;
        std::optional<KHRMaterialIOR> ExtIOR;
        std::optional<KHRMaterialEmissiveStrength> ExtEmissiveStrength;
        std::optional<KHRMaterialIridescence> ExtIridescence;
        std::optional<KHRMaterialSheen> ExtSheen;
        std::optional<KHRMaterialTransmission> ExtTransmission;
        std::optional<KHRMaterialVolume> ExtVolume;
    };

    class Buffer : public Object
    {
    public:

        Buffer(size_t id, size_t bufferSize);
        Buffer(size_t id, FileReader& reader, size_t bufferSize);
        Buffer(size_t id, const std::string& URIData, size_t bufferSize);
        ~Buffer();

        std::vector<uint8_t> Get(size_t byteLength, size_t offset);

        void SetData(uint8_t* data) { m_Data = data; }

        size_t GetByteSize() const { return m_ByteSize; }
    private:
        uint8_t* m_Data = nullptr;
        size_t m_ByteSize = 0;

        friend class OCASI::GLTFImporter;
    };

    struct TRS
    {
        glm::vec3 Translation = glm::vec3(0);
        glm::quat Rotation = glm::quat(1, 0, 0, 0);
        glm::vec3 Scale = glm::vec3(1);
    };

    struct Node : public Object
    {
        Node(size_t index)
            : Object(index)
        {}

        std::string Name;
        size_t Mesh = INVALID_ID;
        std::vector<size_t> Children;

        TRS TrsComponent;
        glm::mat4 LocalTranslationMatrix = glm::mat4(1.0f); // This is only local, when this node is not a root node
        std::vector<float> Weights;
    };

    struct Scene : public Object
    {
        Scene(size_t index)
            : Object(index)
        {}

        std::string Name;
        std::vector<size_t> RootNodes;

    };

    struct Version
    {
        size_t Major = INVALID_ID;
        size_t Minor = INVALID_ID;

        std::string AsString() const
        {
            return FORMAT("{0}.{1}", Major, Minor);
        }
    };

    struct Asset
    {
        Version AssetVersion;
        std::string CopyRight;
        std::string Generator;
        Version MinimumRequiredVersion;

        size_t DefaultSceneIndex = INVALID_ID;

        std::vector<Mesh> Meshes;
        std::vector<Accessor> Accessors;
        std::vector<BufferView> BufferViews;
        std::vector<Material> Materials;
        std::vector<Texture> Textures;
        std::vector<Image> Images;
        std::vector<Sampler> Samplers;
        std::vector<Node> Nodes;
        std::vector<Buffer> Buffers;
        std::vector<Scene> Scenes;
        std::vector<std::string> ExtensionsUsed;
        std::vector<std::string> SupportedExtensionsUsed;
        std::vector<std::string> ExtensionsRequired; // Always empty as I don't support any extensions, that are required

        // TODO: Animations
    };
}