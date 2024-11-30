#pragma once

#include "OCASI/Core/FileUtil.h"

#include "OCASI/Importers/GLTF2/Asset.h"

namespace glz {
    class json_t;
}

namespace OCASI::GLTF {

    class JsonParser
    {
    public:
        JsonParser(OCASI::FileReader& reader);
        JsonParser(OCASI::FileReader& reader, glz::json_t* json);
        ~JsonParser();

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

        bool ParseSparseAccessor(const glz::json_t& jsonAccessor, std::optional<Sparse> &outSparse);
        bool ParseTextureInfo(const glz::json_t& jTextureInfo, std::optional<TextureInfo>& outTextureInfo);

        // Materials
        bool ParsePbrMetallicRoughness(const glz::json_t& jPbrMetallicRoughness, std::optional<PBRMetallicRoughness>& outMaterial);
        // Material extensions
        bool ParsePbrSpecularGlossiness(const glz::json_t& jPbrSpecularGlossiness, std::optional<KHRMaterialPbrSpecularGlossiness>& outMaterial);
        bool ParseSpecular(const glz::json_t& jSpecular, std::optional<KHRMaterialSpecular>& outMaterial);
        bool ParseClearcoat(const glz::json_t& jClearcoat, std::optional<KHRMaterialClearcoat>& outMaterial);
        bool ParseSheen(const glz::json_t& jSheen, std::optional<KHRMaterialSheen>& outMaterial);
        bool ParseTransmission(const glz::json_t& jTransmission, std::optional<KHRMaterialTransmission>& outMaterial);
        bool ParseVolume(const glz::json_t& jVolume, std::optional<KHRMaterialVolume>& outMaterial);
        void ParseIOR(const glz::json_t& jIOR, std::optional<KHRMaterialIOR>& outMaterial);
        void ParseEmissiveStrength(const glz::json_t& jEmissiveStrength, std::optional<KHRMaterialEmissiveStrength>& outMaterial);
        bool ParseIridescence(const glz::json_t& jIridescence, std::optional<KHRMaterialIridescence>& outMaterial);
        bool ParseAnisotropy(const glz::json_t& jAnisotropy, std::optional<KHRMaterialAnisotropy>& outMaterial);

        bool ParsePrimitives(const glz::json_t& jPrimitive, Mesh& mesh);
        void ParseVertexAttributes(const glz::json_t& jVertexAttribute, VertexAttributes& outAttributes);
        void ParseVec3(const glz::json_t& jVec, glm::vec3& out);
        void ParseVec4(const glz::json_t& jVec, glm::vec4& out);
    private:
        FileReader& m_FileReader;

        std::shared_ptr<Asset> m_Asset = nullptr;
        glz::json_t* m_Json = nullptr;
    };

}
