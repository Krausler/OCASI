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
        bool ParseNodes();
        bool ParseScenes();

        bool ParseSparseAccessor(const Json* jsonAccessor, std::optional<Sparse> &outSparse);
        bool ParseTextureInfo(const Json* jsonTextureInfo, std::optional<TextureInfo>& outTextureInfo);

        bool ParsePbrMetallicRoughness(const Json* jsonPbrMetallicRoughness, std::optional<PBRMetallicRoughness>& outMetallicRoughness);
        bool ParsePbrSpecularGlossiness(const Json* jsonExtension, std::optional<KHRMaterialPbrSpecularGlossiness>& outMetallicRoughness);
        bool ParseClearcoat(const Json *jsonExtension, std::optional<KHRMaterialClearcoat>& outMaterial);
        bool ParseSheen(const Json *json, std::optional<KHRMaterialSheen>& outMaterial);
        bool ParseTransmission(const Json *json, std::optional<KHRMaterialTransmission>& outMaterial);
        bool ParseVolume(const Json *json, std::optional<KHRMaterialVolume>& outMaterial);
        void ParseIOR(const Json *json, std::optional<KHRMaterialIOR>& outMaterial);
        void ParseEmissiveStrength(const Json *json, std::optional<KHRMaterialEmissiveStrength>& outMaterial);
        bool ParseIridescence(const Json *json, std::optional<KHRMaterialIridescence>& outMaterial);
        bool ParseAnisotropy(const Json *json, std::optional<KHRMaterialAnisotropy>& outMaterial);
    private:
        FileReader& m_FileReader;

        std::shared_ptr<Asset> m_Asset = nullptr;
        Json* m_Json = nullptr;

    };

}
