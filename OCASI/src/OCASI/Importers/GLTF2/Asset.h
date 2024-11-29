#pragma once

#include "OCASI/Core/Base.h"

#include "OCASI/Core/FileUtil.h"

#include "glm/glm.hpp"
#include "glm/ext/quaternion_float.hpp"

namespace OCASI::GLTF {

    const size_t INVALID_ID = -1;
    const float INVALID_ID_FLOAT = -1;
    const size_t MAX_MIN_ARRAY_SIZE = 16;

    const std::vector<std::string> SUPPORTED_EXTENSIONS = {
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
        Opaque,
        Mask,
        Blend
    };

    enum class MinMagFilter
    {
        Nearest = 9728,
        Linear = 9729,
        NearestMipMapLNearest = 9984,
        NearestMipMapLinear = 9986,
        LinearMipMapNearest = 9985,
        LinearMipMapLinear = 9987
    };

    enum class UVWrap
    {
        ClampToEdge = 33071,
        MirroredRepeat = 33648,
        Repeat = 10497
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
        size_t ByteStride = 0; // Although byteStride does not have a default value, it is easier than making it an optional. It's just adding 0 in a equation which doesn't matter at all.
        // The bufferView.target property is not implemented as it's useless
    };

    struct SparseIndices
    {
        size_t BufferView = INVALID_ID;
        size_t ByteOffset = 0;
        ComponentType componentType = ComponentType::None;
    };

    struct SparseValues
    {
        size_t BufferView = INVALID_ID;
        size_t ByteOffset = 0;
    };

    struct Sparse
    {
        size_t ElementCount = INVALID_ID;
        SparseIndices Indices;
        SparseValues Values;
    };

    struct Accessor : public Object
    {
        Accessor(size_t index)
            : Object(index)
        {}

        std::optional<size_t> BufferView;
        size_t ElementCount = INVALID_ID;
        size_t ByteOffset = 0;
        ComponentType ComponentType = ComponentType::None;
        bool Normalized = false;
        DataType DataType = DataType::None;
        std::array<double, MAX_MIN_ARRAY_SIZE> MinValues;
        std::array<double, MAX_MIN_ARRAY_SIZE> MaxValues;
        std::optional<Sparse> Sparse;
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
        std::optional<size_t> MaterialIndex;
        std::optional<size_t> Indices; // The accessor corresponding to the indices
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

        std::optional<std::string> URI;

        std::optional<size_t> BufferView = INVALID_ID;
        std::optional<std::string> MimeType;
    };

    struct Sampler : public Object
    {
        Sampler(size_t index)
            : Object(index)
        {}

        std::optional<MinMagFilter> MagFilter;
        std::optional<MinMagFilter> MinFilter;
        UVWrap WrapS = UVWrap::Repeat; // Default specified in glTF 2.0 Spec
        UVWrap WrapT = UVWrap::Repeat; // Default specified in glTF 2.0 Spec
    };

    struct Texture : public Object
    {
        Texture(size_t index)
            : Object(index)
        {}

        std::optional<size_t> Sampler;
        std::optional<size_t> Source;
    };

    struct TextureInfo
    {
        size_t Texture = INVALID_ID;
        size_t TexCoords = 0; // Set of texture coordinates
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
        std::optional<float> AttenuationDistance;
        glm::vec3 AttenuationColour = glm::vec3(1.0f, 1.0f, 1.0f);
    };

    struct KHRMaterialIOR
    {
        float Ior = INVALID_ID_FLOAT;
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
        std::optional<TextureInfo> EmissiveTexture;
        glm::vec3 EmissiveColour = glm::vec3(0);

        AlphaMode AlphaMode = AlphaMode::Opaque;
        float AlphaCutoff = 0.5f;

        /// Extensions

        std::optional<KHRMaterialSpecular> Specular;
        std::optional<KHRMaterialPbrSpecularGlossiness> SpecularGlossiness;
        std::optional<KHRMaterialClearcoat> Clearcoat;
        std::optional<KHRMaterialAnisotropy> Anisotropy;
        std::optional<KHRMaterialIOR> IOR;
        std::optional<KHRMaterialEmissiveStrength> EmissiveStrength;
        std::optional<KHRMaterialIridescence> Iridescence;
        std::optional<KHRMaterialSheen> Sheen;
        std::optional<KHRMaterialTransmission> Transmission;
        std::optional<KHRMaterialVolume> Volume;
    };

    class Buffer : public Object
    {
    public:

        Buffer(size_t id, uint8_t* data, size_t bufferSize);
        Buffer(size_t id, FileReader& reader, size_t bufferSize);
        Buffer(size_t id, const std::string& URIData, size_t bufferSize);
        ~Buffer();

        bool SetPointer(size_t position);
        bool AddToPointer(size_t position);

        void* Get(size_t byteLength);

        template<typename T>
        std::vector<T> Get(size_t byteLength)
        {
            void* data = Get(byteLength);
            if (data == nullptr)
                return {};

            std::vector<T> result(byteLength / sizeof(T));
            std::memcpy(result.data(), data, byteLength);
            return result;
        }

        template<typename T>
        std::vector<T> Get(size_t byteLength, size_t byteStride)
        {
            void* data = Get(byteLength);
            if (data == nullptr)
                return {};

            std::vector<T> result;

            for (size_t i = 0; i < byteLength; i += sizeof(T) + byteStride)
            {
                T type;
                std::memcpy(&type, data, sizeof(T));
                result.push_back(type);
            }
            return result;
        }

        size_t GetByteSize() const { return m_ByteSize; }
    private:
        uint8_t* m_Data;

        size_t m_ByteSize = 0;
        size_t m_Pointer = 0;
    };

    struct TRS
    {
        glm::vec3 Translation = glm::vec3(0);
        glm::vec3 Scale = glm::vec3(1);
        glm::quat Rotation = glm::quat(1, 0, 0, 0);
    };

    struct Node : public Object
    {
        Node(size_t index)
            : Object(index)
        {}

        std::string Name;
        std::optional<size_t> Mesh;
        std::vector<size_t> Children;

        std::optional<TRS> TrsComponent;
        std::optional<glm::mat4> LocalTranslationMatrix; // This is only local, when this node is not a root node
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
        uint32_t Major;
        uint32_t Minor;

        std::string AsString() const
        {
            return FORMAT("{0}.{1}", Major, Minor);
        }
    };

    struct Asset
    {
        Version AssetVersion;
        std::optional<std::string> CopyRight;
        std::optional<std::string> Generator;
        std::optional<Version> MinimumRequiredVersion;

        std::optional<size_t> DefaultSceneIndex;

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