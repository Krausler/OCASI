#pragma once

#include "OCASI/Core/FileUtil.h"

#include "OCASI/Importers/GLTF2/Asset.h"

namespace simdjson::fallback::ondemand {
    
    class array;
    class object;
    
}

namespace OCASI::GLTF {
    struct Json;
    
    class JsonParser
    {
    public:
        JsonParser(OCASI::FileReader& reader, Json* json);
        ~JsonParser();

        std::shared_ptr<Asset> ParseGLTFTextFile();
    private:
        void ParseAssetDescription(); // This is for parsing the Scene's generator and required version
        void ParseExtensions();
        void ParseBuffers();
        void ParseBufferViews();
        void ParseAccessors();
        void ParseSparseAccessor(simdjson::fallback::ondemand::object& jsonAccessor, std::optional<Sparse> &outSparse);
        void ParseImages();
        void ParseSamplers();
        void ParseTextures();
        void ParseTextureInfo(simdjson::fallback::ondemand::object& jObject, std::string_view name, std::optional<TextureInfo>& outTextureInfo);
        void ParseMaterials();
        void ParseMeshes();
        void ParsePrimitives(simdjson::fallback::ondemand::array& jPrimitives, Mesh& mesh);
        void ParseNodes();
        void ParseScenes();

        // Materials
        void ParsePbrMetallicRoughness(simdjson::fallback::ondemand::object& jPbrMetallicRoughness, std::optional<PBRMetallicRoughness>& outMaterial);
        // Material extensions
        void ParsePbrSpecularGlossiness(simdjson::fallback::ondemand::object& jPbrSpecularGlossiness, std::optional<KHRMaterialPbrSpecularGlossiness>& outSpecularGlossiness);
        void ParseSpecular(simdjson::fallback::ondemand::object& jSpecular, std::optional<KHRMaterialSpecular>& outMaterial);
        void ParseClearcoat(simdjson::fallback::ondemand::object& jClearcoat, std::optional<KHRMaterialClearcoat>& outMaterial);
        void ParseSheen(simdjson::fallback::ondemand::object& jSheen, std::optional<KHRMaterialSheen>& outMaterial);
        void ParseTransmission(simdjson::fallback::ondemand::object& jTransmission, std::optional<KHRMaterialTransmission>& outMaterial);
        void ParseVolume(simdjson::fallback::ondemand::object& jVolume, std::optional<KHRMaterialVolume>& outMaterial);
        void ParseIOR(simdjson::fallback::ondemand::object& jIOR, std::optional<KHRMaterialIOR>& outMaterial);
        void ParseEmissiveStrength(simdjson::fallback::ondemand::object& jEmissiveStrength, std::optional<KHRMaterialEmissiveStrength>& outMaterial);
        void ParseIridescence(simdjson::fallback::ondemand::object& jIridescence, std::optional<KHRMaterialIridescence>& outMaterial);
        void ParseAnisotropy(simdjson::fallback::ondemand::object& jAnisotropy, std::optional<KHRMaterialAnisotropy>& outMaterial);

        // Mesh data loading
        void ParseVertexAttributes(simdjson::fallback::ondemand::object& jVertexAttributes, VertexAttributes& outAttributes);
        void ParseVec3(simdjson::fallback::ondemand::object& jObject, std::string_view name, glm::vec3& out);
        void ParseVec4(simdjson::fallback::ondemand::object& jObject, std::string_view name, glm::vec4& out);
    private:
        FileReader& m_FileReader;

        std::shared_ptr<Asset> m_Asset = nullptr;
        Json* m_Json = nullptr;
    };

}
