#pragma once

#include "OCASI/Core/FileUtil.h"

#include "OCASI/Importers/GLTF2/Asset.h"

namespace OCASI {
    struct Json;
}

namespace OCASI::GLTF {

    class TextFileParser
    {
    public:
        TextFileParser(OCASI::FileReader& reader);
        ~TextFileParser();

        std::shared_ptr<Asset> ParseGLTFTextFile();
    private:
        bool ParseAssetDescription(); // This is for parsing the Scene's generator and required version
        bool ParseExtensions();
        bool ParseBuffers();
        bool ParseBufferViews();
        bool ParseAccessors();
        void ParseImages();
        void ParseSamplers();
        void ParseTextures();
        bool ParseMaterials();
        bool ParseMeshes();
        void ParseNodes();
        bool ParseScenes();

        bool ParseSparseAccessor(const Json* jsonAccessor, std::optional<Sparse> &outSparse);
        bool ParseTextureInfo(const Json* jsonTextureInfo, std::optional<TextureInfo>& outTextureInfo);

        // Materials
        bool ParsePbrMetallicRoughness(const Json* jsonPbrMetallicRoughness, std::optional<PBRMetallicRoughness>& outMaterial);
        // Material extensions
        bool ParsePbrSpecularGlossiness(const Json* jsonPbrSpecularGlossiness, std::optional<KHRMaterialPbrSpecularGlossiness>& outMaterial);
        bool ParseSpecular(const Json* jsonSpecular, std::optional<KHRMaterialSpecular>& outMaterial);
        bool ParseClearcoat(const Json* jsonClearcoat, std::optional<KHRMaterialClearcoat>& outMaterial);
        bool ParseSheen(const Json* jsonSheen, std::optional<KHRMaterialSheen>& outMaterial);
        bool ParseTransmission(const Json* jsonTransmission, std::optional<KHRMaterialTransmission>& outMaterial);
        bool ParseVolume(const Json* jsonVolume, std::optional<KHRMaterialVolume>& outMaterial);
        void ParseIOR(const Json* jsonIOR, std::optional<KHRMaterialIOR>& outMaterial);
        void ParseEmissiveStrength(const Json* jsonEmissiveStrength, std::optional<KHRMaterialEmissiveStrength>& outMaterial);
        bool ParseIridescence(const Json* jsonIridescence, std::optional<KHRMaterialIridescence>& outMaterial);
        bool ParseAnisotropy(const Json* jsonAnisotropy, std::optional<KHRMaterialAnisotropy>& outMaterial);

        bool ParsePrimitives(const Json* jsonPrimitive, Mesh& mesh);
        void ParseVertexAttributes(const Json* jsonVertexAttribute, VertexAttributes& outAttributes);
        void ParseVec3(const Json* jsonVec, glm::vec3& out);
        void ParseVec4(const Json* jsonVec, glm::vec4& out);
    private:
        FileReader& m_FileReader;

        std::shared_ptr<Asset> m_Asset = nullptr;
        Json* m_Json = nullptr;

    };

}
