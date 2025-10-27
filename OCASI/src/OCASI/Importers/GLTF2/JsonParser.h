#pragma once

#include "OCASI/Importers/GLTF2/Asset.h"

#include "OCBase/IO/FileStream.h"

namespace simdjson::fallback::ondemand {
    
    class array;
    class object;
    
}

namespace OCASI::GLTF {
    struct Json;
    
    class JsonParser
    {
    public:
        JsonParser(OCBase::FileStreamReader& reader, Json* json);
        ~JsonParser();

        ExpectedImportT<SharedPtr<Asset>> ParseGLTFTextFile();
    private:
        
        ExpectedImport ParseAssetDescription(); // This is for parsing the Scene's generator and required version
        ExpectedImport ParseExtensions();
        ExpectedImport ParseBuffers();
        ExpectedImport ParseBufferViews();
        ExpectedImport ParseAccessors();
        ExpectedImport ParseSparseAccessor(simdjson::fallback::ondemand::object& jsonAccessor, std::optional<Sparse> &outSparse);
        void ParseImages();
        void ParseSamplers();
        void ParseTextures();
        ExpectedImportT<std::optional<TextureInfo>> ParseTextureInfo(simdjson::fallback::ondemand::object& jObject, std::string_view name);
        ExpectedImport ParseMaterials();
        ExpectedImport ParseMeshes();
        ExpectedImport ParsePrimitives(simdjson::fallback::ondemand::array& jPrimitives, Mesh& mesh);
        ExpectedImport ParseNodes();
        ExpectedImport ParseScenes();

        // Materials
        ExpectedImportT<PBRMetallicRoughness> ParsePbrMetallicRoughness(simdjson::fallback::ondemand::object& jPbrMetallicRoughness);
        // Material extensions
        ExpectedImportT<KHRMaterialPbrSpecularGlossiness> ParsePbrSpecularGlossiness(simdjson::fallback::ondemand::object& jPbrSpecularGlossiness);
        ExpectedImportT<KHRMaterialSpecular> ParseSpecular(simdjson::fallback::ondemand::object& jSpecular);
        ExpectedImportT<KHRMaterialClearcoat> ParseClearcoat(simdjson::fallback::ondemand::object& jClearcoat);
        ExpectedImportT<KHRMaterialSheen> ParseSheen(simdjson::fallback::ondemand::object& jSheen);
        ExpectedImportT<KHRMaterialTransmission> ParseTransmission(simdjson::fallback::ondemand::object& jTransmission);
        ExpectedImportT<KHRMaterialVolume> ParseVolume(simdjson::fallback::ondemand::object& jVolume);
        KHRMaterialIOR ParseIOR(simdjson::fallback::ondemand::object& jIOR);
        KHRMaterialEmissiveStrength ParseEmissiveStrength(simdjson::fallback::ondemand::object& jEmissiveStrength);
        ExpectedImportT<KHRMaterialIridescence> ParseIridescence(simdjson::fallback::ondemand::object& jIridescence);
        ExpectedImportT<KHRMaterialAnisotropy> ParseAnisotropy(simdjson::fallback::ondemand::object& jAnisotropy);

        // Mesh data loading
        ExpectedImportT<VertexAttributes> ParseVertexAttributes(simdjson::fallback::ondemand::object& jVertexAttributes);
        ExpectedImportT<glm::vec3> ParseVec3(simdjson::fallback::ondemand::object& jObject, std::string_view name);
        ExpectedImportT<glm::vec4> ParseVec4(simdjson::fallback::ondemand::object& jObject, std::string_view name);
    private:
        OCBase::FileStreamReader& m_FileReader;

        SharedPtr<Asset> m_Asset = nullptr;
        Json* m_Json = nullptr;
    };

}
