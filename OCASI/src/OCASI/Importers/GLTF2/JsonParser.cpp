#include "JsonParser.h"

#include "OCASI/Core/StringUtil.h"

#include "OCASI/Importers/GLTF2/Json.h"

#include "OCBase/IO/IO.h"

// Apparently __LINE__ has to be parsed around 2 times
#define OCASI_FAIL_ON_SIMDJSON_ERROR_IMPL(err, code, msg, line) error_code error##line = err; if(error##line) { return std::unexpected(ImportError(code, msg)); }
#define OCASI_FAIL_ON_SIMDJSON_ERROR_IMPL2(err, code, msg, line) OCASI_FAIL_ON_SIMDJSON_ERROR_IMPL(err, code, msg, line)

#define OCASI_FAIL_ON_SIMDJSON_ERROR(err, code, msg) OCASI_FAIL_ON_SIMDJSON_ERROR_IMPL2(err, code, msg, __LINE__)
#define OCASI_FAIL_IF_OBJ_NOT_EXISTS(json, requiredParam, outValue, msg) OCASI_FAIL_ON_SIMDJSON_ERROR(json[requiredParam].get(outValue), ImportError::Type::MissingParameter, msg)

#define OCASI_HAS_PROPERTY(json, parameter, outValue) if (!json[parameter].get(outValue))
#define OCASI_SET_PROPERTY_IF_EXISTS(json, parameter, outValue) json[parameter].get(outValue)
#define OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(json, parameter, T, outValue) json[parameter].get<T>().get(outValue)

#define OCASI_RETURN_SELF_IF_ERROR(err) if (auto e = err; !e.has_value()) return e;

#define OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT_IMPL(funcName, json, name, outParam, captures, line) auto exp##line = funcName(json, name).transform([captures](const auto& val) { outParam = val; }); \
                                                                                                      if (!exp##line.has_value()) return UnexpectedF(exp##line.error())

#define OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT_IMPL2(funcName, json, name, outParam, captures, line) OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT_IMPL(funcName, json, name, outParam, captures, line)
#define OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(funcName, json, name, outParam, captures) OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT_IMPL2(funcName, json, name, outParam, captures, __LINE__)

using namespace simdjson;

namespace OCASI::GLTF {

    const std::string_view EXTENSIONS_USED_PROPERTY = "extensionsUsed";
    const std::string_view EXTENSIONS_REQUIRED_PROPERTY = "extensionsRequired";
    const std::string_view ACCESSORS_PROPERTY = "accessors";
    const std::string_view BUFFER_VIEWS_PROPERTY = "bufferViews";
    const std::string_view BUFFERS_PROPERTY = "buffers";
    const std::string_view NODES_PROPERTY = "nodes";
    const std::string_view ASSET_PROPERTY = "asset";
    const std::string_view MESHES_PROPERTY = "meshes";
    const std::string_view MATERIALS_PROPERTY = "materials";
    const std::string_view TEXTURES_PROPERTY = "textures";
    const std::string_view IMAGES_PROPERTY = "images";
    const std::string_view SAMPLERS_PROPERTY = "samplers";
    const std::string_view SCENE_PROPERTY = "scene";
    const std::string_view SCENES_PROPERTY = "scenes";

    JsonParser::JsonParser(OCBase::FileStreamReader& reader, Json* json)
        : m_FileReader(reader), m_Json(json)
    {}

    JsonParser::~JsonParser()
    {
        delete m_Json;
    }

    ExpectedImportT<SharedPtr<Asset>> JsonParser::ParseGLTFTextFile()
    {
        m_Asset = MakeShared<Asset>();
        ondemand::document& json = m_Json->Get();

        // The order of how things are parsed doesn't really matter, however, it does make sense
        // to first read in the 'asset' and 'extensions', followed by all data objects and ending
        // with the connecting objects, like meshes nodes and scenes, in order to simulate a
        // scenario where the order of things would be mandatory.

        // Read the project generator and version
        if (auto e = ParseAssetDescription(); !e)
            return UnexpectedF(e.error());
        
        if (auto e = ParseExtensions(); !e)
            return UnexpectedF(e.error());

        OCASI_SET_PROPERTY_IF_EXISTS(json, SCENE_PROPERTY, m_Asset->DefaultSceneIndex);

        if (auto e = ParseBuffers(); !e)
            return UnexpectedF(e.error());
        
        if (auto e = ParseBufferViews(); !e)
            return UnexpectedF(e.error());
        
        if (auto e = ParseAccessors(); !e)
            return UnexpectedF(e.error());

        ParseImages();
        ParseSamplers();
        ParseTextures();
        
        if (auto e = ParseMaterials(); !e)
            return UnexpectedF(e.error());
        
        if (auto e = ParseMeshes(); !e)
            return UnexpectedF(e.error());
        
        if (auto e = ParseNodes(); !e)
            return UnexpectedF(e.error());
        
        if (auto e = ParseScenes(); !e)
            return UnexpectedF(e.error());

        return m_Asset;
    }
    
    ExpectedImport JsonParser::ParseAssetDescription()
    {
        auto& json = m_Json->Get();

        ondemand::object jAsset;
        OCASI_FAIL_IF_OBJ_NOT_EXISTS(json, ASSET_PROPERTY, jAsset, "Required 'asset' object not present in GLTF file, though mandatory");
        
        // The jAsset files version
        std::string_view strVersion;
        OCASI_FAIL_IF_OBJ_NOT_EXISTS(jAsset, "version", strVersion, "Required 'version' property in jAsset is not present, though mandatory");

        String version(strVersion);
        m_Asset->AssetVersion = {};
        m_Asset->AssetVersion.Major = std::stoi(version.substr(0, version.find('.')));
        m_Asset->AssetVersion.Minor = std::stoi(version.substr(version.find('.') + 1));
        OCASI_ASSERT(m_Asset->AssetVersion.Major == 2);

        std::string_view minVersionStr;
        OCASI_HAS_PROPERTY(jAsset, "version", minVersionStr)
        {
            String minVersion(minVersionStr);
            m_Asset->MinimumRequiredVersion.Major = std::stoi(minVersion.substr(0, minVersion.find('.')));
            m_Asset->MinimumRequiredVersion.Minor = std::stoi(minVersion.substr(minVersion.find('.') + 1));
        }

        std::string_view generator;
        OCASI_HAS_PROPERTY(jAsset, "generator", generator)
        {
            m_Asset->Generator = generator;
        }
        
        std::string_view copyright;
        OCASI_HAS_PROPERTY(jAsset, "copyright", generator)
        {
            m_Asset->CopyRight = copyright;
        }
        
        return ExpectedImport();
    }
    
    ExpectedImport JsonParser::ParseExtensions()
    {
        auto& json = m_Json->Get();

        // Return when there are no extensions required
        ondemand::array jExtensions;
        if (json[EXTENSIONS_USED_PROPERTY].get(jExtensions))
            return ExpectedImport();
        
        for (auto jExt : jExtensions)
        {
            std::string_view extName;
            OCASI_FAIL_ON_SIMDJSON_ERROR(jExt.get(extName), ImportError::Type::MissingParameter, "Failed to get jExtensions name.");
            
            m_Asset->ExtensionsUsed.push_back(std::move(std::string(extName)));

            if (std::find(SUPPORTED_EXTENSIONS.begin(), SUPPORTED_EXTENSIONS.end(), extName) == SUPPORTED_EXTENSIONS.end())
                m_Asset->SupportedExtensionsUsed.push_back(std::move(std::string(extName)));

        }

        // OCASI currently does not support any extensions REQUIRED,
        // so we return when there is an 'extensionRequired' section.
        if (!json[EXTENSIONS_REQUIRED_PROPERTY].error())
            return UnexpectedF(ImportError(ImportError::Type::UnsupportedFeature, "The GLTF importer currently does NOT support required extensions."));
        
        return ExpectedImport();
    }
    
    ExpectedImport JsonParser::ParseBuffers()
    {
        auto& json = m_Json->Get();

        ondemand::array jBuffers;
        if (json[BUFFERS_PROPERTY].get(jBuffers))
            return ExpectedImport();
        
        size_t i = 0;
        for (auto jBuffer : jBuffers)
        {
            size_t byteLength;
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jBuffer, "byteLength", byteLength, "Required 'byteLength' property in jBuffer is not present, though mandatory.");
            
            std::string_view data;
            OCASI_HAS_PROPERTY(jBuffer, "uri", data)
            {
                String s(data);
                if (Util::StartsWith(s, "data:"))
                {
                    String uri = s.substr(s.find(':'));
                    m_Asset->Buffers.emplace_back(i, uri, byteLength);
                }
                else
                {
                    Path bufferDataFile = m_FileReader.GetFile().GetPath().parent_path() / s;
                    ExpectedImport error = OCBase::IO::Open(bufferDataFile, OCBase::FileMode::Read | OCBase::FileMode::Bin)
                            .transform([this, i, byteLength](const OCBase::File& f)
                            {
                                OCBase::FileStreamReader reader(const_cast<OCBase::File&>(f));
                                m_Asset->Buffers.emplace_back(i, reader, byteLength);
                                reader.GetFile().Close();
                            })
                            .or_else([i](const OCBase::FileError& error)
                            {
                                return ExpectedImport(UnexpectedF(ImportError(ImportError::Type::File, error.GetErrorMessage())));
                            });
                    
                    if(!error)
                        return error;
                }
            }
            else
                m_Asset->Buffers.emplace_back(i, byteLength);
            
            i++;
        }
        return ExpectedImport();
    }
    
    ExpectedImport JsonParser::ParseBufferViews()
    {
        auto& json = m_Json->Get();

        ondemand::array jBufferViews;
        if (json[BUFFER_VIEWS_PROPERTY].get(jBufferViews))
            return ExpectedImport();

        size_t i = 0;
        for (auto jBufferView : jBufferViews)
        {
            
            BufferView& bufferView = m_Asset->BufferViews.emplace_back(i);
            
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jBufferView, "buffer", bufferView.Buffer, "Required 'buffer' property in bufferView is not present, though mandatory");
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jBufferView, "byteLength", bufferView.ByteLength, "Required 'byteLength' property in bufferView is not present, though mandatory");
            
            OCASI_SET_PROPERTY_IF_EXISTS(jBufferView, "byteOffset", bufferView.ByteOffset);
            OCASI_SET_PROPERTY_IF_EXISTS(jBufferView, "byteStride", bufferView.ByteStride);
            i++;
        }
        
        return ExpectedImport();
    }
    
    ExpectedImport JsonParser::ParseAccessors()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jAccessors;
        if (json[ACCESSORS_PROPERTY].get(jAccessors))
            return ExpectedImport();
        
        size_t i = 0;
        for (auto jAccessor : jAccessors)
        {
            Accessor& accessor = m_Asset->Accessors.emplace_back(i);
            
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jAccessor, "count", accessor.ElementCount, "Required 'count' property in accessor is not present, though mandatory");
            std::string_view dataType;
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jAccessor, "type", dataType, "Required 'type' property in accessor is not present, though mandatory");
            size_t compType;
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jAccessor, "componentType", compType, "Required 'componentType' property in accessor is not present, though mandatory");

            accessor.CompType = (ComponentType) compType;

            // Reading the data type
            {
                if (dataType == "SCALAR")
                    accessor.Type = DataType::Scalar;
                else if (dataType == "VEC2")
                    accessor.Type = DataType::Vec2;
                else if (dataType == "VEC3")
                    accessor.Type = DataType::Vec3;
                else if (dataType == "VEC4")
                    accessor.Type = DataType::Vec4;
                else if (dataType == "MAT2")
                    accessor.Type = DataType::Mat2;
                else if (dataType == "MAT3")
                    accessor.Type = DataType::Mat3;
                else if (dataType == "MAT4")
                    accessor.Type = DataType::Mat4;
                else
                {
                    return UnexpectedF(ImportError(ImportError::Type::InvalidParameter, FORMAT("Invalid data format {} in accessor {}", dataType, i)));
                }
            }
            
            OCASI_SET_PROPERTY_IF_EXISTS(jAccessor, "bufferView", accessor.BufferView);
            OCASI_SET_PROPERTY_IF_EXISTS(jAccessor, "byteOffset", accessor.ByteOffset);
            OCASI_SET_PROPERTY_IF_EXISTS(jAccessor, "normalized", accessor.Normalized);

            ondemand::array jMax;
            OCASI_HAS_PROPERTY(jAccessor, "max", jMax)
            {
                size_t j = 0;
                for (auto jMaxVal : jMax)
                {
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jMaxVal.get(accessor.MaxValues.at(j)), ImportError::Type::ReadMalfunction, "Failed to get 'max' property values.");
                    j++;
                }
            }
            
            ondemand::array jMin;
            OCASI_HAS_PROPERTY(jAccessor, "min", jMin)
            {
                size_t j = 0;
                for (auto jMinVal : jMin)
                {
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jMinVal.get(accessor.MinValues.at(j)), ImportError::Type::ReadMalfunction, "Failed to get 'min' property values.");
                    j++;
                }
            }
            
            ondemand::object jSparse;
            // TODO: Come back to here
            OCASI_HAS_PROPERTY(jAccessor, "sparse", jSparse)
            {
                ExpectedImport error = ParseSparseAccessor(jSparse, accessor.SparseAccessor = Sparse());
                if(error)
                    return error;
            }
            
            i++;
        }
        return ExpectedImport();
    }
    
    // A sparse accessor specifies that a part of an 'accessors' data is replaced with data from a different 'bufferView'.
    ExpectedImport JsonParser::ParseSparseAccessor(simdjson::ondemand::object& jSparse, std::optional<Sparse>& outSparse)
    {
        auto& json = m_Json->Get();
        
        OCASI_FAIL_IF_OBJ_NOT_EXISTS(jSparse, "count", outSparse->ElementCount, "Required 'count' property in sparse accessor is not present, though mandatory.");
        ondemand::object jSparseIndices;
        OCASI_FAIL_IF_OBJ_NOT_EXISTS(jSparse, "indices", jSparseIndices, "Required 'indices' property in sparse accessor is not present, though mandatory.");
        ondemand::object jSparseValues;
        OCASI_FAIL_IF_OBJ_NOT_EXISTS(jSparse, "values", jSparseValues, "Required 'values' property in sparse accessor is not present, though mandatory.");
        
        // Indices: Specifies the range of values to be replaced.
        {
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jSparseIndices, "bufferView", outSparse->Indices.BufferView, "Required 'bufferView' property in sparse accessor indices is not present, though mandatory.")
            size_t compType;
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jSparseIndices, "componentType", compType, "Required 'componentType' property in sparse accessor indices is not present, though mandatory.")
            
            OCASI_SET_PROPERTY_IF_EXISTS(jSparseIndices, "byteOffset", outSparse->Indices.ByteOffset);
        }
        
        // Values: Specifies the values, used to override the values of 'indices'.
        {
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jSparseValues, "bufferView", outSparse->Values.BufferView, "Required 'bufferView' property in sparse accessor values is not present, though mandatory.")
            OCASI_SET_PROPERTY_IF_EXISTS(jSparseValues, "byteOffset", outSparse->Values.ByteOffset);
        }
        return ExpectedImport();
    }
    
    void JsonParser::ParseImages()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jImages;
        if (json[IMAGES_PROPERTY].get(jImages))
            return;

        size_t i = 0;
        for (auto jImage : jImages)
        {
            
            Image& image = m_Asset->Images.emplace_back(i);
            
            std::string_view val;
            OCASI_SET_PROPERTY_IF_EXISTS(jImage, "uri", val);
            image.URI = val;
            OCASI_SET_PROPERTY_IF_EXISTS(jImage, "mimeType", val);
            image.MimeType = val;
            OCASI_SET_PROPERTY_IF_EXISTS(jImage, "bufferView", image.BufferView);
            i++;
        }
    }
    
    void JsonParser::ParseSamplers()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jSamplers;
        if (json[SAMPLERS_PROPERTY].get(jSamplers))
            return;
        
        size_t i = 0;
        for (auto jSampler : jSamplers)
        {
            Sampler& sampler = m_Asset->Samplers.emplace_back(i);
            
            size_t val = 0;
            OCASI_SET_PROPERTY_IF_EXISTS(jSampler, "magFilter", val);
            sampler.MagFilter = (MinMagFilter) val;
            OCASI_SET_PROPERTY_IF_EXISTS(jSampler, "minFilter", val);
            sampler.MinFilter = (MinMagFilter) val;
            
            OCASI_SET_PROPERTY_IF_EXISTS(jSampler, "wrapS", val);
            sampler.WrapS = (UVWrap) val;
            OCASI_SET_PROPERTY_IF_EXISTS(jSampler, "wrapT", val);
            sampler.WrapT = (UVWrap) val;
            i++;
        }
    }
    
    void JsonParser::ParseTextures()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jTextures;
        if (json[TEXTURES_PROPERTY].get(jTextures))
            return;
        
        size_t i = 0;
        for (auto jTexture : jTextures)
        {
            Texture& texture = m_Asset->Textures.emplace_back(i);
            OCASI_SET_PROPERTY_IF_EXISTS(jTexture, "source", texture.Source);
            OCASI_SET_PROPERTY_IF_EXISTS(jTexture, "sampler", texture.Sampler);
            i++;
        }
    }
    
    ExpectedImportT<std::optional<TextureInfo>> JsonParser::ParseTextureInfo(simdjson::fallback::ondemand::object& jObject, std::string_view name)
    {
        std::optional<TextureInfo> info;
        
        ondemand::object jTextureInfo;
        OCASI_HAS_PROPERTY(jObject, name, jTextureInfo)
        {
            info = TextureInfo();
            
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jTextureInfo, "index", info->Texture, "Required 'index' property in texture info is not present, though mandatory");
            
            OCASI_SET_PROPERTY_IF_EXISTS(jTextureInfo, "texCoord", info->TexCoords);
            OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jTextureInfo, "scale", float, info->Scale);
            OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jTextureInfo, "strength", float, info->Scale);
        }
        
        return info;
    }
    
    ExpectedImport JsonParser::ParseMaterials()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jMaterials;
        if (json[MATERIALS_PROPERTY].get(jMaterials))
            return ExpectedImport();
        
        size_t i = 0;
        for (auto rJMaterial : jMaterials)
        {
            ondemand::object jMaterial;
            OCASI_FAIL_ON_SIMDJSON_ERROR(rJMaterial.get(jMaterial), ImportError::Type::ReadMalfunction, "Failed to get texture object");
            Material& material = m_Asset->Materials.emplace_back(i);
            
            std::string_view name;
            OCASI_SET_PROPERTY_IF_EXISTS(jMaterial, "name", name);
            material.Name = name;
            
            ondemand::object jPbrMetallicRoughness;
            OCASI_HAS_PROPERTY(jMaterial, "pbrMetallicRoughness", jPbrMetallicRoughness)
            {
                auto ePbrMetallicRoughness = ParsePbrMetallicRoughness(jPbrMetallicRoughness)
                        .transform([&material](const PBRMetallicRoughness& pbr) { material.MetallicRoughness = pbr; });
                if (!ePbrMetallicRoughness)
                    return UnexpectedF(ePbrMetallicRoughness.error());
            }
            
            // Normal texture
            auto eNormalTex = ParseTextureInfo(jMaterial, "normalTexture")
                    .transform([&material](const std::optional<TextureInfo>& tex) { material.NormalTexture = tex; });
            if (!eNormalTex)
                return UnexpectedF(eNormalTex.error());
            
            // Occlusion texture
            auto eOcclusionTex = ParseTextureInfo(jMaterial, "occlusionTexture")
                    .transform([&material](const std::optional<TextureInfo>& tex) { material.OcclusionTexture = tex; });
            if (!eOcclusionTex)
                return UnexpectedF(eOcclusionTex.error());
            
            // Emissive texture
            auto eEmissiveTex = ParseTextureInfo(jMaterial, "emissiveTexture")
                    .transform([&material](const std::optional<TextureInfo>& tex) { material.EmissiveTexture = tex; });
            if (!eEmissiveTex)
                return UnexpectedF(eEmissiveTex.error());
            // Emissive factor
            auto eEmissiveFactor = ParseVec3(jMaterial, "emissiveFactor")
                    .transform([&material](const glm::vec3& colour) { material.EmissiveColour = colour; });
            
            
            std::string_view alphaMode;
            OCASI_HAS_PROPERTY(jMaterial, "alphaMode", alphaMode)
            {
                if (alphaMode == "OPAQUE")
                    material.AMode = AlphaMode::Opaque;
                else if (alphaMode == "MASK")
                    material.AMode = AlphaMode::Mask;
                else if (alphaMode == "BLEND")
                    material.AMode = AlphaMode::Blend;
                else
                    return UnexpectedF(ImportError(ImportError::Type::InvalidParameter, FORMAT("Invalid alphaMode parameter: {}", alphaMode)));
            }
            
            auto t  = jMaterial.at_path("").get<float>();
            
            OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jMaterial, "alphaCutoff", float, material.AlphaCutoff);
            OCASI_SET_PROPERTY_IF_EXISTS(jMaterial, "doubleSided", material.IsDoubleSided);
            
            /// Extensions
            ondemand::array jExtensions;
            OCASI_HAS_PROPERTY(jMaterial, "extensions", jExtensions)
            {
                size_t j = 0;
                for (auto rJExt : jExtensions)
                {
                    ondemand::object jExt;
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jExtensions.at(i).get(jExt), ImportError::Type::ReadMalfunction, "Failed to get extension.");
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_pbrSpecularGlossiness", jExt)
                    {
                        auto ePbr = ParsePbrSpecularGlossiness(jExt).transform([&material](const auto& val) { material.ExtSpecularGlossiness = val; });
                        if (!ePbr)
                            return UnexpectedF(ePbr.error());
                    }
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_specular", jExt)
                    {
                        auto eSpecular = ParseSpecular(jExt).transform([&material](const auto& val) { material.ExtSpecular = val; });
                        if (!eSpecular)
                            return UnexpectedF(eSpecular.error());
                    }
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_clearcoat", jExt)
                    {
                        auto eClearCoat = ParseClearcoat(jExt).transform([&material](const auto& val) { material.ExtClearcoat = val; });
                        if (!eClearCoat)
                            return UnexpectedF(eClearCoat.error());
                    }
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_sheen", jExt)
                    {
                        auto eSheen = ParseSheen(jExt).transform([&material](const auto& val) { material.ExtSheen = val; });
                        if (!eSheen)
                            return UnexpectedF(eSheen.error());
                    }
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_transmission", jExt)
                    {
                        auto eTransmission = ParseTransmission(jExt).transform([&material](const auto& val) { material.ExtTransmission = val; });
                        if (!eTransmission)
                            return UnexpectedF(eTransmission.error());
                    }
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_volume", jExt)
                    {
                        auto eVolume = ParseVolume(jExt).transform([&material](const auto& val) { material.ExtVolume = val; });
                        if (!eVolume)
                            return UnexpectedF(eVolume.error());
                    }
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_ior", jExt)
                        material.ExtIOR = ParseIOR(jExt);
                    
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_emissive_strength", jExt)
                    {
                        material.ExtEmissiveStrength = ParseEmissiveStrength(jExt);
                    }
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_iridescence", jExt)
                    {
                        auto eIridescence = ParseIridescence(jExt).transform([&material](const auto& val) { material.ExtIridescence = val; });
                        if (!eIridescence)
                            return UnexpectedF(eIridescence.error());
                    }
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_anisotropy", jExt)
                    {
                        auto eAnisotropy = ParseAnisotropy(jExt).transform([&material](const auto& val) { material.ExtAnisotropy = val; });
                        if (!eAnisotropy)
                            return UnexpectedF(eAnisotropy.error());
                    }
                    j++;
                }
            }
            i++;
        }
        return ExpectedImport();
    }
    
    ExpectedImport JsonParser::ParseMeshes()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jMeshes;
        if (json[MESHES_PROPERTY].get(jMeshes))
            return ExpectedImport();

        size_t i = 0;
        for (auto jMesh : jMeshes)
        {
            Mesh& mesh = m_Asset->Meshes.emplace_back(i);
            
            ondemand::array jPrimitives;
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jMesh, "primitives", jPrimitives, "Required 'primitives' property in mesh is not present, though mandatory");
            
            ParsePrimitives(jPrimitives, mesh);
            
            ondemand::array jWeights;
            OCASI_HAS_PROPERTY(jMesh, "weights", jWeights)
            {
                for (auto jWeight : jWeights)
                {
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jWeight.get<float>().get(mesh.Weights.emplace_back()), ImportError::Type::ReadMalfunction, "Failed to get mesh weight.");
                }
            }
            i++;
        }
        return ExpectedImport();
    }
    
    ExpectedImport JsonParser::ParsePrimitives(simdjson::fallback::ondemand::array& jPrimitives, Mesh& mesh)
    {
        size_t i = 0;
        for (auto jPrimitive : jPrimitives)
        {
            Primitive& primitive = mesh.Primitives.emplace_back(i);
            
            ondemand::object jAttributes;
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jPrimitive, "attributes", jAttributes, "Required 'primitives' property in mesh primitive is not present, though mandatory");
            auto eVertAttribs = ParseVertexAttributes(jAttributes)
                    .transform([&primitive](const VertexAttributes& vertexAttributes) { primitive.Attributes = vertexAttributes; });
            if (!eVertAttribs)
                return UnexpectedF(eVertAttribs.error());
            
            OCASI_SET_PROPERTY_IF_EXISTS(jPrimitive, "indices", primitive.Indices);
            OCASI_SET_PROPERTY_IF_EXISTS(jPrimitive, "material", primitive.MaterialIndex);
            size_t mode = 0;
            OCASI_HAS_PROPERTY(jPrimitive, "mode", mode)
            {
                primitive.Type = (PrimitiveType) mode;
            }
            
            ondemand::array jTargets;
            OCASI_HAS_PROPERTY(jPrimitive, "targets", jTargets)
            {
                primitive.MorphTargets.reserve(jTargets.count_elements());
                for (auto rJTarget : jTargets)
                {
                    ondemand::object jTarget;
                    OCASI_FAIL_ON_SIMDJSON_ERROR(rJTarget.get(jTarget), ImportError::Type::ReadMalfunction, "Failed to get morph target.");
                    auto eVertAttribsMorph = ParseVertexAttributes(jTarget)
                            .transform([&primitive](const VertexAttributes& attribs) { primitive.MorphTargets.emplace_back(attribs); });
                }
            }
        }
        return ExpectedImport();
    }
    
    ExpectedImport JsonParser::ParseNodes()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jNodes;
        if (json[NODES_PROPERTY].get(jNodes))
            return ExpectedImport();

        size_t i = 0;
        for (auto rJNode : jNodes)
        {
            ondemand::object jNode;
            OCASI_FAIL_ON_SIMDJSON_ERROR(rJNode.get(jNode), ImportError::Type::ReadMalfunction, "Failed to get node.");
            Node& node = m_Asset->Nodes.emplace_back(i);

            // Cameras and animations are not supported
            ondemand::array jChildren;
            OCASI_HAS_PROPERTY(jNode, "children", jChildren)
            {
                for (auto jChild : jChildren)
                {
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jChild.get(node.Children.emplace_back()), ImportError::Type::ReadMalfunction, "Failed to get node child.")
                }
            }
            
            std::string_view name;
            OCASI_SET_PROPERTY_IF_EXISTS(jNode, "name", name);
            node.Name = name;
            
            OCASI_SET_PROPERTY_IF_EXISTS(jNode, "mesh", node.Mesh);
            
            // TRS components
            OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseVec3, jNode, "translation", node.TrsComponent.Translation, &node);
            
            glm::vec4 rotVec;
            OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseVec4, jNode, "rotation", rotVec, &rotVec);
            node.TrsComponent.Rotation = glm::quat(rotVec.w, rotVec.x, rotVec.y, rotVec.z);
            
            OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseVec3, jNode, "scale", node.TrsComponent.Scale, &node);
            
            // Matrix
            ondemand::array jMatrix;
            OCASI_HAS_PROPERTY(jNode, "matrix", jMatrix)
            {
                size_t j = 0;
                for (auto jMatrixVal : jMatrix)
                {
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jMatrixVal.get<float>().get(node.LocalTranslationMatrix[j % 4][j / 4]), ImportError::Type::ReadMalfunction, "Failed to get matrix value.");
                    j++;
                }
                
                OCASI_ASSERT(j == 16);
            }
            
            ondemand::array jWeights;
            OCASI_HAS_PROPERTY(jNode, "weights", jWeights)
            {
                for (auto jWeight : jWeights)
                {
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jWeight.get<float>().get(node.Weights.emplace_back()), ImportError::Type::ReadMalfunction, "Failed to get weight value.");
                }
            }
            i++;
        }
        return ExpectedImport();
    }
    
    ExpectedImport JsonParser::ParseScenes()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jScenes;
        if (json[SCENES_PROPERTY].get(jScenes))
            return ExpectedImport();

        size_t i = 0;
        for (auto jScene : jScenes)
        {
            Scene& scene = m_Asset->Scenes.emplace_back(i);
            
            std::string_view name;
            OCASI_SET_PROPERTY_IF_EXISTS(jScene, "name", name);
            scene.Name = name;
            
            ondemand::array jRootNodes;
            OCASI_HAS_PROPERTY(jScene, "nodes", jRootNodes)
            {
                size_t j = 0;
                for (auto jNode : jRootNodes)
                {
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jNode.get(scene.RootNodes.emplace_back()), ImportError::Type::ReadMalfunction, "Failed to get root node.");
                    j++;
                }
            }
            i++;
        }
        return ExpectedImport();
    }
    
    ExpectedImportT<PBRMetallicRoughness> JsonParser::ParsePbrMetallicRoughness(simdjson::fallback::ondemand::object& jPbrMetallicRoughness)
    {
        PBRMetallicRoughness metallicRoughness = {};
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jPbrMetallicRoughness, "metallicFactor", float, metallicRoughness.Metallic);
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jPbrMetallicRoughness, "roughnessFactor", float, metallicRoughness.Roughness);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jPbrMetallicRoughness, "metallicRoughnessTexture", metallicRoughness.MetallicRoughnessTexture, &metallicRoughness);
        
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseVec4, jPbrMetallicRoughness, "baseColourFactor", metallicRoughness.BaseColour, &metallicRoughness);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jPbrMetallicRoughness, "baseColorTexture", metallicRoughness.BaseColourTexture, &metallicRoughness);
        
        return metallicRoughness;
    }
    
    ExpectedImportT<KHRMaterialPbrSpecularGlossiness> JsonParser::ParsePbrSpecularGlossiness(simdjson::fallback::ondemand::object& jPbrSpecularGlossiness)
    {
        KHRMaterialPbrSpecularGlossiness specularGlossiness = {};
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jPbrSpecularGlossiness, "glossinessFactor", float, specularGlossiness.GlossinessFactor);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jPbrSpecularGlossiness, "specularGlossinessTexture", specularGlossiness.SpecularGlossinessTexture, &specularGlossiness);
        
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseVec4, jPbrSpecularGlossiness, "diffuseFactor", specularGlossiness.DiffuseFactor, &specularGlossiness);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jPbrSpecularGlossiness, "diffuseTexture", specularGlossiness.DiffuseTexture, &specularGlossiness);
        
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseVec3, jPbrSpecularGlossiness, "specularFactor", specularGlossiness.SpecularFactor, &specularGlossiness);
        return specularGlossiness;
    }
    
    ExpectedImportT<KHRMaterialSpecular> JsonParser::ParseSpecular(simdjson::fallback::ondemand::object& jSpecular)
    {
        KHRMaterialSpecular specular = {};
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jSpecular, "specularFactor", float, specular.SpecularFactor);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jSpecular, "specularTexture", specular.SpecularTexture, &specular);
        
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseVec3, jSpecular, "specularColorFactor", specular.SpecularColourFactor, &specular);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jSpecular, "specularColorTexture", specular.SpecularColourTexture, &specular);
        
        return specular;
    }
    
    ExpectedImportT<KHRMaterialClearcoat> JsonParser::ParseClearcoat(simdjson::fallback::ondemand::object& jClearCoat)
    {
        KHRMaterialClearcoat clearCoat = {};
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jClearCoat, "clearcoatFactor", float, clearCoat.ClearcoatFactor);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jClearCoat, "clearcoatTexture", clearCoat.ClearcoatTexture, &clearCoat);
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jClearCoat, "clearcoatRoughnessFactor", float, clearCoat.ClearcoatRoughnessFactor);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jClearCoat, "clearcoatRoughnessTexture", clearCoat.ClearcoatRoughnessTexture, &clearCoat);
        
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jClearCoat, "clearcoatNormalTexture", clearCoat.ClearcoatNormalTexture, &clearCoat);
        
        return clearCoat;
    }
    
    ExpectedImportT<KHRMaterialSheen> JsonParser::ParseSheen(simdjson::fallback::ondemand::object& jSheen)
    {
        KHRMaterialSheen sheen = {};
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jSheen, "sheenRoughnessFactor", float, sheen.SheenRoughnessFactor);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jSheen, "sheenRoughnessTexture", sheen.SheenRoughnessTexture, &sheen);
        
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseVec3, jSheen, "sheenColorFactor", sheen.SheenColourFactor, &sheen);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jSheen, "sheenColourTexture", sheen.SheenColourTexture, &sheen);
        
        return sheen;
        
    }
    
    ExpectedImportT<KHRMaterialTransmission> JsonParser::ParseTransmission(simdjson::fallback::ondemand::object& jTransmission)
    {
        KHRMaterialTransmission transmission = {};
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jTransmission, "transmissionFactor", float, transmission.TransmissionFactor);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jTransmission, "transmissionTexture", transmission.TransmissionTexture, &transmission);
        
        return transmission;
    }
    
    ExpectedImportT<KHRMaterialVolume> JsonParser::ParseVolume(simdjson::fallback::ondemand::object& jVolume)
    {
        KHRMaterialVolume volume = {};
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jVolume, "thicknessFactor", float, volume.ThicknessFactor);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jVolume, "thicknessTexture", volume.ThicknessTexture, &volume);
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jVolume, "attenuationDistance", float, volume.AttenuationDistance);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseVec3, jVolume, "attenuationColor", volume.AttenuationColour, &volume);
        
        return volume;
    }
    
    KHRMaterialIOR JsonParser::ParseIOR(simdjson::fallback::ondemand::object& jIOR)
    {
        KHRMaterialIOR ior = {};
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jIOR, "ior", float, ior.IOR);
        return ior;
    }
    
    KHRMaterialEmissiveStrength JsonParser::ParseEmissiveStrength(simdjson::fallback::ondemand::object& jEmissiveStrength)
    {
        KHRMaterialEmissiveStrength emissiveStrength = {};
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jEmissiveStrength, "emissiveStrength", float, emissiveStrength.EmissiveStrength);
        return emissiveStrength;
    }
    
    ExpectedImportT<KHRMaterialIridescence> JsonParser::ParseIridescence(simdjson::fallback::ondemand::object& jIridescence)
    {
        KHRMaterialIridescence iridescence;
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jIridescence, "iridescenceFactor", float, iridescence.IridescenceFactor);
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jIridescence, "iridescenceIor", float, iridescence.IridescenceIor);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jIridescence, "iridescenceTexture", iridescence.IridescenceTexture, &iridescence);
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jIridescence, "iridescenceThicknessMinimum", float, iridescence.IridescenceThicknessMinimum);
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jIridescence, "iridescenceThicknessMaximum", float, iridescence.IridescenceThicknessMaximum);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jIridescence, "iridescenceThicknessTexture", iridescence.IridescenceThicknessTexture, &iridescence);
        
        return iridescence;
    }
    
    ExpectedImportT<KHRMaterialAnisotropy> JsonParser::ParseAnisotropy(simdjson::fallback::ondemand::object& jAnisotropy)
    {
        KHRMaterialAnisotropy anisotropy;
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jAnisotropy, "anisotropyFactor", float, anisotropy.AnisotropyFactor);
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseTextureInfo, jAnisotropy, "anisotropyTexture", anisotropy.AnisotropyTexture, &anisotropy);
        
        OCASI_SIMPLIFY_EXPECTED_ASSIGNMENT(ParseVec3, jAnisotropy, "anisotropyDirection", anisotropy.AnisotropyDirection, &anisotropy);
        return anisotropy;
    }
    
    ExpectedImportT<VertexAttributes> JsonParser::ParseVertexAttributes(simdjson::fallback::ondemand::object& jVertexAttributes)
    {
        VertexAttributes attributes = {};
        
        for (auto jAttribute : jVertexAttributes)
        {
            std::string_view key;
            OCASI_FAIL_ON_SIMDJSON_ERROR(jAttribute.escaped_key().get(key), ImportError::Type::ReadMalfunction, "Failed to get vertex attribute key.");
            OCASI_FAIL_ON_SIMDJSON_ERROR(jAttribute.value().get(attributes[std::string(key)]), ImportError::Type::ReadMalfunction, "Failed to get vertex attribute value.");
        }
        
        return attributes;
    }
    
    ExpectedImportT<glm::vec3> JsonParser::ParseVec3(simdjson::fallback::ondemand::object& jObject, std::string_view name)
    {
        glm::vec3 vec;
        
        ondemand::array jVec;
        OCASI_HAS_PROPERTY(jObject, name, jVec)
        {
            size_t i = 0;
            for (auto jVecVal : jVec) {
                
                float element;
                OCASI_FAIL_ON_SIMDJSON_ERROR(jVecVal.get<float>().get(element), ImportError::Type::ReadMalfunction, "Failed to parse vec3 array element.");
                
                vec[(int)i] = element;
                
                i++;
            }
            OCASI_ASSERT(i == 3);
        }
        
        return vec;
    }
    
    ExpectedImportT<glm::vec4> JsonParser::ParseVec4(simdjson::fallback::ondemand::object& jObject, std::string_view name)
    {
        glm::vec4 vec;
        
        ondemand::array jVec;
        OCASI_HAS_PROPERTY(jObject, name, jVec)
        {
            size_t i = 0;
            for (auto jVecVal : jVec) {
                
                float element;
                OCASI_FAIL_ON_SIMDJSON_ERROR(jVecVal.get<float>().get(element), ImportError::Type::ReadMalfunction, "Failed to parse vec4 array element");
                
                vec[(int)i] = element;
                
                i++;
            }
            OCASI_ASSERT(i == 4);
        }
        
        return vec;
    }
}