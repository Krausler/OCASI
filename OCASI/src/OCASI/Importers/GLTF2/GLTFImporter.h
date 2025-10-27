#pragma once

#include "OCASI/Core/BaseImporter.h"

#include "OCASI/Importers/GLTF2/JsonParser.h"

namespace OCASI {
    struct Json;

    struct GLBHeader
    {
        uint32_t Magic; // binary header expands to "glTF" string
        uint32_t Version; // Is only valid if it equals 2
        uint32_t FileLength; // The spec defines a maximum file size of 4GB, however, for simplicity std::numerical_limits<uint32_t>::max() is used
    };

    struct GLBChunk
    {
        uint32_t ChunkLength; // The chunk length in bytes
        uint32_t Type; // Maybe use an enum for this
        uint8_t* Data; // The chunks data with byte length of ChunkLength
    };

    class GLTFImporter : public BaseImporter
    {
    public:
        virtual bool CanLoad(OCBase::FileStreamReader& reader) override;
        virtual ExpectedImportT<SharedPtr<Scene>> Load3DFile(OCBase::FileStreamReader& reader) override;
        
        virtual const Vector<std::string_view> GetSupportedFileExtensions() const override { return { ".gltf", ".glb" }; }
        virtual ImporterType GetImporterType() const override { return ImporterType::GLTF; }
    private:
        ExpectedImport LoadBinary();
        GLBChunk LoadChunk();
        
        ExpectedImport ConvertToOCASIScene();

        bool CheckBinaryHeader();
        void CreateNodes(size_t sceneIndex);
        void TraverseNodes(GLTF::Node& gltfNode, SharedPtr<Node> ocasiNode);
        ExpectedImport CreateMesh(size_t meshIndex);
        ExpectedImport CreateMaterial(size_t materialIndex);
        ExpectedImportT<SharedPtr<Image>> CreateTexture(std::optional<GLTF::TextureInfo>& texInfo);
        ExpectedImportT<Vector<uint8_t>> GetAccessorData(size_t accessorIndex);
        ExpectedImportT<std::span<uint8_t>> GetBufferViewData(size_t bufferViewIndex, size_t accessorOffset, size_t& outByteStride);

        FilterOption ConvertMinMagFilterToFilterOption(GLTF::MinMagFilter filter);
        FaceType ConvertPrimitiveTypeToFaceType(GLTF::PrimitiveType primitive);
        ImageType ConvertMimeTypeToImagType(const String& mimeType);
    private:
        OCBase::FileStreamReader* m_FileReader = nullptr;
        GLTF::Json* m_Json = nullptr;
        
        SharedPtr<GLTF::Asset> m_Asset = nullptr;
        SharedPtr<Scene> m_Scene = nullptr;
    };

}
