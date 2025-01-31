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
        bool ParseAssetDescription(); // This is for parsing the Scene's generator and required version
        bool ParseExtensions();
        bool ParseBuffers();
        bool ParseBufferViews();
        bool ParseAccessors();
        bool ParseImages();
        bool ParseSamplers();
        bool ParseTextures();
        bool ParseMaterials();
        bool ParseMeshes();
        bool ParseNodes();
        bool ParseScenes();

        bool ParseSparseAccessor(simdjson::fallback::ondemand::object& jsonAccessor, std::optional<Sparse> &outSparse);
        bool ParseTextureInfo(simdjson::fallback::ondemand::object& jObject, std::string_view name, std::optional<TextureInfo>& outTextureInfo);

        // Materials
        bool ParsePbrMetallicRoughness(simdjson::fallback::ondemand::object& jPbrMetallicRoughness, std::optional<PBRMetallicRoughness>& outMaterial);
        // Material extensions
        bool ParsePbrSpecularGlossiness(simdjson::fallback::ondemand::object& jPbrSpecularGlossiness, std::optional<KHRMaterialPbrSpecularGlossiness>& outSpecularGlossiness);
        bool ParseSpecular(simdjson::fallback::ondemand::object& jSpecular, std::optional<KHRMaterialSpecular>& outMaterial);
        bool ParseClearcoat(simdjson::fallback::ondemand::object& jClearcoat, std::optional<KHRMaterialClearcoat>& outMaterial);
        bool ParseSheen(simdjson::fallback::ondemand::object& jSheen, std::optional<KHRMaterialSheen>& outMaterial);
        bool ParseTransmission(simdjson::fallback::ondemand::object& jTransmission, std::optional<KHRMaterialTransmission>& outMaterial);
        bool ParseVolume(simdjson::fallback::ondemand::object& jVolume, std::optional<KHRMaterialVolume>& outMaterial);
        bool ParseIOR(simdjson::fallback::ondemand::object& jIOR, std::optional<KHRMaterialIOR>& outMaterial);
        bool ParseEmissiveStrength(simdjson::fallback::ondemand::object& jEmissiveStrength, std::optional<KHRMaterialEmissiveStrength>& outMaterial);
        bool ParseIridescence(simdjson::fallback::ondemand::object& jIridescence, std::optional<KHRMaterialIridescence>& outMaterial);
        bool ParseAnisotropy(simdjson::fallback::ondemand::object& jAnisotropy, std::optional<KHRMaterialAnisotropy>& outMaterial);

        // Mesh data loading
        bool ParsePrimitives(simdjson::fallback::ondemand::array& jPrimitives, Mesh& mesh);
        bool ParseVertexAttributes(simdjson::fallback::ondemand::object& jVertexAttributes, VertexAttributes& outAttributes);
        bool ParseVec3(simdjson::fallback::ondemand::object& jObject, std::string_view name, glm::vec3& out);
        bool ParseVec4(simdjson::fallback::ondemand::object& jObject, std::string_view name, glm::vec4& out);
    private:
        FileReader& m_FileReader;

        std::shared_ptr<Asset> m_Asset = nullptr;
        Json* m_Json = nullptr;
    };

}
