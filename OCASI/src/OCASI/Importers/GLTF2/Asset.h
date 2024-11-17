#pragma once

#include "OCASI/Core/Base.h"

#include "glm/glm.hpp"
#include "glm/ext/quaternion_float.hpp"

namespace OCASI {

    const size_t INVALID_ID = -1;
    const size_t MAX_MIN_MAX_ARRAY_SIZE = 16;

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

    constexpr size_t ByteSize(ComponentType);

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

    enum class PrimitiveType : size_t
    {
        None = INVALID_ID,
        Point = 0,
        Line = 1,
        LineLoop = 2,
        LineStrip = 3,
        Triangle = 4,
        TriangleStrip = 5,
        TriangleFan = 6
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
        size_t ByteStride = 0;
        // The bufferView.target property is not implemented as it is useless
    };

    struct SparseIndices
    {
        size_t BufferView = INVALID_ID;
        size_t ByteOffset = 0;
        ComponentType componentType = ComponentType::None;
    };

    struct SparseValue
    {
        size_t BufferView = INVALID_ID;
        size_t BufferOffset = 0;
    };

    struct Sparse
    {
        size_t Count;
        SparseIndices Indices;
        SparseValue Value;
    };

    struct Accessor : public Object
    {
        Accessor(size_t index)
            : Object(index)
        {}

        size_t BufferView = INVALID_ID;
        size_t ByteOffset = 0;
        ComponentType ComponentType = ComponentType::None;
        bool Normalized = false;
        size_t ElementCount = INVALID_ID;
        DataType DataType = DataType::None;
        std::array<double, MAX_MIN_MAX_ARRAY_SIZE> MinValues;
        std::array<double, MAX_MIN_MAX_ARRAY_SIZE> MaxValues;
        std::optional<Sparse> Sparse;
    };

    using VertexAttributes = std::unordered_map<std::string, size_t>;

    // The primitive structs holds indices into the bufferView array
    struct Primitive : public Object
    {
        Primitive(size_t index)
            : Object(index)
        {}

        PrimitiveType Type = PrimitiveType::None;
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

        uint32_t MagFilter;
        uint32_t MinFilter;
        uint32_t WrapS = 10497; // Default specified in glTF 2.0 Spec
        uint32_t WrapT = 10497; // Default specified in glTF 2.0 Spec
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
        size_t TexCoords = 0;
    };

    struct PBRMetallicRoughness
    {
        glm::vec4 BaseColor;
        float Metallic = 0.0f;
        float Roughness = 0.0f;

        TextureInfo BaseColorTexture;
        bool HasBaseColorTexture;
        TextureInfo MetallicRoughnessTexture;
        bool HasMetallicRoughnessTexture;
    };

    struct Material
    {
        std::string Name;
        PBRMetallicRoughness MetallicRoughness;

        bool IsDoubleSided;
        TextureInfo NormalTexture;
        float NormalScale;
        TextureInfo OcclusionTexture;
        TextureInfo EmissiveTexture;
        glm::vec3 EmissiveColor;
        std::string AlphaMode;
        float AlphaCutoff;
    };

    class Buffer : public Object
    {
    public:

        Buffer(size_t id, std::vector<uint8_t>& data, size_t BufferSize);
        Buffer(size_t id, const std::string& URI, size_t BufferSize);
        Buffer(size_t id, size_t BufferSize);

        void SetPointer(size_t position);
        void AddToPointer(size_t position);

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
        std::vector<uint8_t> m_Data;
        size_t m_ByteSize; // The size is already stored in the vectors size component, however the glTF 2.0 Spec states, that the specified data may be bigger than it's defined size
        std::vector<uint8_t>::iterator m_Pointer;
        std::vector<uint8_t>::iterator m_End;
    };

    struct Node : public Object
    {
        Node(size_t index)
            : Object(index)
        {}

        std::string Name;
        size_t Mesh = INVALID_ID;
        std::vector<size_t> Children;

        glm::vec3 Translation;
        glm::vec3 Scale;
        glm::quat Rotation;
        glm::mat4 LocalTranslationMatrix; // This is only local, the current node is not a root node
    };

    struct Scene : public Object
    {
        Scene(size_t index)
            : Object(index)
        {}

        std::vector<Mesh> Meshes;
        std::vector<Accessor> Accessors;
        std::vector<BufferView> BufferViews;
        std::vector<Material> Materials;
        std::vector<Texture> Textures;
        std::vector<Image> Images;
        std::vector<Sampler> Samplers;

        // TODO: Animations

        std::vector<Node> Nodes;
        std::vector<Buffer> Buffers;
    };

    struct Version
    {
        uint32_t Major = 2;
        uint32_t Minor = 0;

        std::string AsString() const
        {
            return FORMAT("{0}.{1}", Major, Minor);
        }
    };

    struct Asset
    {
        std::string CopyRight;
        std::string Generator;
        Version AssetVersion;
        Version MinimumRequiredVersion;

        std::vector<Scene> Scenes;
    };
}