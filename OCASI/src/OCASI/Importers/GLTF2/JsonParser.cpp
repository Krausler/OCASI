#include "JsonParser.h"

#include "OCASI/Core/StringUtil.h"

#include "OCASI/Importers/GLTF2/Json.h"

// Apparently __LINE__ has to be parsed around 2 times
#define OCASI_FAIL_ON_SIMDJSON_ERROR_IMPL(err, msg, line) error_code error##line = err; if (error##line) { throw OCASI::FailedImportError(msg); }
#define OCASI_FAIL_ON_SIMDJSON_ERROR_IMPL2(err, msg, line) OCASI_FAIL_ON_SIMDJSON_ERROR_IMPL(err, msg, line)

#define OCASI_FAIL_ON_SIMDJSON_ERROR(err, msg) OCASI_FAIL_ON_SIMDJSON_ERROR_IMPL2(err, msg, __LINE__)
#define OCASI_FAIL_IF_OBJ_NOT_EXISTS(json, requiredParam, outValue, msg) OCASI_FAIL_ON_SIMDJSON_ERROR(json[requiredParam].get(outValue), msg)

#define OCASI_HAS_PROPERTY(json, parameter, outValue) if (!json[parameter].get(outValue))
#define OCASI_SET_PROPERTY_IF_EXISTS(json, parameter, outValue) json[parameter].get(outValue)
#define OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(json, parameter, T, outValue) json[parameter].get<T>().get(outValue)

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

    JsonParser::JsonParser(FileReader& reader, Json* json)
        : m_FileReader(reader), m_Json(json)
    {}

    JsonParser::~JsonParser()
    {
        delete m_Json;
    }

    std::shared_ptr<Asset> JsonParser::ParseGLTFTextFile()
    {
        m_Asset = MakeShared<Asset>();
        ondemand::document& json = m_Json->Get();

        // The order of how things are parsed doesn't really matter, however, it does make sense
        // to first read in asset and extensions, followed by all data objects and ending with
        // the connecting objects, like meshes nodes and scenes, in order to simulate scenario
        // where the order of things would be fundamental.

        // Read the project generator and version
        ParseAssetDescription();
        ParseExtensions();

        OCASI_SET_PROPERTY_IF_EXISTS(json, SCENE_PROPERTY, m_Asset->DefaultSceneIndex);

        ParseBuffers();
        ParseBufferViews();
        ParseAccessors();

        ParseImages();
        ParseSamplers();
        ParseTextures();

        ParseMaterials();
        ParseMeshes();

        ParseNodes();
        ParseScenes();

        return m_Asset;
    }
    
    void JsonParser::ParseAssetDescription()
    {
        auto& json = m_Json->Get();

        ondemand::object jAsset;
        OCASI_FAIL_IF_OBJ_NOT_EXISTS(json, ASSET_PROPERTY, jAsset, "Required 'asset' object not present in GLTF file, though mandatory")
        
        // The jAsset files version
        std::string_view strVersion;
        OCASI_FAIL_IF_OBJ_NOT_EXISTS(jAsset, "version", strVersion, "Required 'version' property in jAsset is not present, though mandatory");

        std::string version(strVersion);
        m_Asset->AssetVersion = {};
        m_Asset->AssetVersion.Major = std::stoi(version.substr(0, version.find('.')));
        m_Asset->AssetVersion.Minor = std::stoi(version.substr(version.find('.') + 1));
        OCASI_ASSERT(m_Asset->AssetVersion.Major == 2);

        std::string_view minVersionStr;
        OCASI_HAS_PROPERTY(jAsset, "version", minVersionStr)
        {
            std::string minVersion(minVersionStr);
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
    }
    
    void JsonParser::ParseExtensions()
    {
        auto& json = m_Json->Get();

        // Return when there are no jExtensions required
        ondemand::array jExtensions;
        if (json[EXTENSIONS_USED_PROPERTY].get(jExtensions))
            return;
        
        for (auto jExt : jExtensions)
        {
            std::string_view extName;
            OCASI_FAIL_ON_SIMDJSON_ERROR(jExt.get(extName), "Failed to get jExtensions name: {}");
            
            m_Asset->ExtensionsUsed.push_back(std::move(std::string(extName)));

            if (std::find(SUPPORTED_EXTENSIONS.begin(), SUPPORTED_EXTENSIONS.end(), extName) == SUPPORTED_EXTENSIONS.end())
                m_Asset->SupportedExtensionsUsed.push_back(std::move(std::string(extName)));

        }

        // OCASI currently does not support any jExtensions that are required, so we return when there is an 'extensionRequired' section
        if (!json[EXTENSIONS_REQUIRED_PROPERTY].error())
            throw FailedImportError("The GLTF importer currently does not support any extensions that are mandatory for parsing.");
    }
    
    void JsonParser::ParseBuffers()
    {
        auto& json = m_Json->Get();

        ondemand::array jBuffers;
        if (json[BUFFERS_PROPERTY].get(jBuffers))
            return;
        
        size_t i = 0;
        for (auto jBuffer : jBuffers)
        {
            size_t byteLength;
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jBuffer, "byteLength", byteLength, "Required 'byteLength' property in jBuffer is not present, though mandatory.");
            
            std::string_view data;
            OCASI_HAS_PROPERTY(jBuffer, "uri", data)
            {
                std::string s(data);
                if (Util::StartsWith(s, "data:"))
                {
                    std::string uri = s.substr(s.find(':'));
                    m_Asset->Buffers.emplace_back(i, uri, byteLength);
                }
                else
                {
                    FileReader binFileReader(m_FileReader.GetParentPath() / data, true);
                    m_Asset->Buffers.emplace_back(i, binFileReader, byteLength);
                }
            }
            else
            {
                m_Asset->Buffers.emplace_back(i, byteLength);
            }
            i++;
        }
    }
    
    void JsonParser::ParseBufferViews()
    {
        auto& json = m_Json->Get();

        ondemand::array jBufferViews;
        if (json[BUFFER_VIEWS_PROPERTY].get(jBufferViews))
            return;

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
    }
    
    void JsonParser::ParseAccessors()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jAccessors;
        if (json[ACCESSORS_PROPERTY].get(jAccessors))
            return;
        
        size_t i = 0;
        for (auto jAccessor : jAccessors)
        {
            Accessor& accessor = m_Asset->Accessors.emplace_back(i);
            
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jAccessor, "count", accessor.ElementCount, "Required 'byteLength' property in accessor is not present, though mandatory");
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
                    throw FailedImportError(FORMAT("Unsupported accessor data type {}", dataType));
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
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jMaxVal.get(accessor.MaxValues.at(j)), "Failed to get 'max' property value.");
                    j++;
                }
            }
            
            ondemand::array jMin;
            OCASI_HAS_PROPERTY(jAccessor, "min", jMin)
            {
                size_t j = 0;
                for (auto jMinVal : jMin)
                {
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jMinVal.get(accessor.MinValues.at(j)), "Failed to get 'min' property value.");
                    j++;
                }
            }
            
            ondemand::object jSparse;
            OCASI_HAS_PROPERTY(jAccessor, "sparse", jSparse)
                ParseSparseAccessor(jSparse, accessor.Sparse = Sparse());
            
            i++;
        }
    }
    
    void JsonParser::ParseSparseAccessor(simdjson::ondemand::object& jSparse, std::optional<Sparse>& outSparse)
    {
        auto& json = m_Json->Get();
        
        OCASI_FAIL_IF_OBJ_NOT_EXISTS(jSparse, "count", outSparse->ElementCount, "Required 'count' property in sparse accessor is not present, though mandatory.");
        ondemand::object jSparseIndices;
        OCASI_FAIL_IF_OBJ_NOT_EXISTS(jSparse, "indices", jSparseIndices, "Required 'indices' property in sparse accessor is not present, though mandatory.");
        ondemand::object jSparseValues;
        OCASI_FAIL_IF_OBJ_NOT_EXISTS(jSparse, "values", jSparseValues, "Required 'values' property in sparse accessor is not present, though mandatory.");
        
        // Indices
        {
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jSparseIndices, "bufferView", outSparse->Indices.BufferView, "Required 'bufferView' property in sparse accessor indices is not present, though mandatory.")
            size_t compType;
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jSparseIndices, "componentType", compType, "Required 'componentType' property in sparse accessor indices is not present, though mandatory.")
            
            OCASI_SET_PROPERTY_IF_EXISTS(jSparseIndices, "byteOffset", outSparse->Indices.ByteOffset);
        }
        
        // Values
        {
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jSparseValues, "bufferView", outSparse->Values.BufferView, "Required 'bufferView' property in sparse accessor values is not present, though mandatory.")
            OCASI_SET_PROPERTY_IF_EXISTS(jSparseValues, "byteOffset", outSparse->Values.ByteOffset);
        }
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
    
    void JsonParser::ParseTextureInfo(simdjson::fallback::ondemand::object& jObject, std::string_view name, std::optional<TextureInfo>& outTextureInfo)
    {
        ondemand::object jTextureInfo;
        OCASI_HAS_PROPERTY(jObject, name, jTextureInfo)
        {
            outTextureInfo = TextureInfo();
            
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jTextureInfo, "index", outTextureInfo->Texture, "Required 'index' property in texture info is not present, though mandatory");
            
            OCASI_SET_PROPERTY_IF_EXISTS(jTextureInfo, "texCoord", outTextureInfo->TexCoords);
            OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jTextureInfo, "scale", float, outTextureInfo->Scale);
            OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jTextureInfo, "strength", float, outTextureInfo->Scale);
        }
        
        return;
    }
    
    void JsonParser::ParseMaterials()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jMaterials;
        if (json[MATERIALS_PROPERTY].get(jMaterials))
            return;
        
        size_t i = 0;
        for (auto rJMaterial : jMaterials)
        {
            ondemand::object jMaterial;
            OCASI_FAIL_ON_SIMDJSON_ERROR(rJMaterial.get(jMaterial), "Failed to get texture object");
            Material& material = m_Asset->Materials.emplace_back(i);
            
            std::string_view name;
            OCASI_SET_PROPERTY_IF_EXISTS(jMaterial, "name", name);
            material.Name = name;
            
            ondemand::object jPbrMetallicRoughness;
            OCASI_HAS_PROPERTY(jMaterial, "pbrMetallicRoughness", jPbrMetallicRoughness)
                ParsePbrMetallicRoughness(jPbrMetallicRoughness, material.MetallicRoughness = PBRMetallicRoughness());
            
            ParseTextureInfo(jMaterial, "normalTexture", material.NormalTexture);
            ParseTextureInfo(jMaterial, "occlusionTexture", material.OcclusionTexture);
            ParseTextureInfo(jMaterial, "emissiveTexture", material.EmissiveTexture);
            ParseVec3(jMaterial, "emissiveFactor", material.EmissiveColour);
            
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
                {
                    throw FailedImportError(FORMAT("Unsupported alphaMode option {}.", alphaMode));
                }
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
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jExtensions.at(i).get(jExt), "Failed to get extension");
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_pbrSpecularGlossiness", jExt)
                        ParsePbrSpecularGlossiness(jExt, material.ExtSpecularGlossiness = KHRMaterialPbrSpecularGlossiness());
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_specular", jExt)
                        ParseSpecular(jExt, material.ExtSpecular = KHRMaterialSpecular());
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_clearcoat", jExt)
                        ParseClearcoat(jExt, material.ExtClearcoat = KHRMaterialClearcoat());
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_sheen", jExt)
                        ParseSheen(jExt, material.ExtSheen = KHRMaterialSheen());
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_transmission", jExt)
                        ParseTransmission(jExt, material.ExtTransmission = KHRMaterialTransmission());
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_volume", jExt)
                        ParseVolume(jExt, material.ExtVolume = KHRMaterialVolume());
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_ior", jExt)
                        ParseIOR(jExt, material.ExtIOR = KHRMaterialIOR());
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_emissive_strength", jExt)
                        ParseEmissiveStrength(jExt, material.ExtEmissiveStrength = KHRMaterialEmissiveStrength());
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_iridescence", jExt)
                        ParseIridescence(jExt, material.ExtIridescence = KHRMaterialIridescence());
                    
                    OCASI_HAS_PROPERTY(jMaterial, "KHR_materials_anisotropy", jExt)
                        ParseAnisotropy(jExt, material.ExtAnisotropy = KHRMaterialAnisotropy());
                    j++;
                }
            }
            i++;
        }
    }
    
    void JsonParser::ParseMeshes()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jMeshes;
        if (json[MESHES_PROPERTY].get(jMeshes))
            return;

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
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jWeight.get<float>().get(mesh.Weights.emplace_back()), "Failed to get mesh weight");
                }
            }
            i++;
        }
    }
    
    void JsonParser::ParsePrimitives(simdjson::fallback::ondemand::array& jPrimitives, Mesh& mesh)
    {
        size_t i = 0;
        for (auto jPrimitive : jPrimitives)
        {
            Primitive& primitive = mesh.Primitives.emplace_back(i);
            
            ondemand::object jAttributes;
            OCASI_FAIL_IF_OBJ_NOT_EXISTS(jPrimitive, "attributes", jAttributes, "Required 'primitives' property in mesh primitive is not present, though mandatory");
            ParseVertexAttributes(jAttributes, primitive.Attributes);
            
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
                size_t j = 0;
                for (auto rJTarget : jTargets)
                {
                    ondemand::object jTarget;
                    OCASI_FAIL_ON_SIMDJSON_ERROR(rJTarget.get(jTarget), "Failed to get morph target");
                    ParseVertexAttributes(jTarget, primitive.MorphTargets.emplace_back(j));
                    j++;
                }
            }
        }
    }
    
    void JsonParser::ParseNodes()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jNodes;
        if (json[NODES_PROPERTY].get(jNodes))
            return;

        size_t i = 0;
        for (auto rJNode : jNodes)
        {
            ondemand::object jNode;
            OCASI_FAIL_ON_SIMDJSON_ERROR(rJNode.get(jNode), "Failed to get node");
            Node& node = m_Asset->Nodes.emplace_back(i);

            // Cameras and animations are not supported
            ondemand::array jChildren;
            OCASI_HAS_PROPERTY(jNode, "children", jChildren)
            {
                for (auto jChild : jChildren)
                {
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jChild.get(node.Children.emplace_back()), "Failed to get node child")
                }
            }
            
            std::string_view name;
            OCASI_SET_PROPERTY_IF_EXISTS(jNode, "name", name);
            node.Name = name;
            
            OCASI_SET_PROPERTY_IF_EXISTS(jNode, "mesh", node.Mesh);
            
            ParseVec3(jNode, "translation", node.TrsComponent.Translation);
            glm::vec4 rotVec;
            ParseVec4(jNode, "rotation", rotVec);
            node.TrsComponent.Rotation = glm::quat(rotVec.w, rotVec.x, rotVec.y, rotVec.z);
            ParseVec3(jNode, "scale", node.TrsComponent.Scale);
            
            ondemand::array jMatrix;
            OCASI_HAS_PROPERTY(jNode, "matrix", jMatrix)
            {
                size_t j = 0;
                for (auto jMatrixVal : jMatrix)
                {
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jMatrixVal.get<float>().get(node.LocalTranslationMatrix[j % 4][j / 4]), "Failed to get matrix value");
                    j++;
                }
                
                OCASI_ASSERT(j == 16);
            }
            
            ondemand::array jWeights;
            OCASI_HAS_PROPERTY(jNode, "weights", jMatrix)
            {
                for (auto jWeight : jWeights)
                {
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jWeight.get<float>().get(node.Weights.emplace_back()), "Failed to get matrix value");
                }
            }
            i++;
        }
    }
    
    void JsonParser::ParseScenes()
    {
        auto& json = m_Json->Get();
        
        ondemand::array jScenes;
        if (json[SCENES_PROPERTY].get(jScenes))
            return;

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
                    OCASI_FAIL_ON_SIMDJSON_ERROR(jNode.get(scene.RootNodes.emplace_back()), "Failed to get root node");
                    j++;
                }
            }
            i++;
        }
    }
    
    void JsonParser::ParsePbrMetallicRoughness(simdjson::fallback::ondemand::object& jPbrMetallicRoughness, std::optional<PBRMetallicRoughness>& outMetallicRoughness)
    {
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jPbrMetallicRoughness, "metallicFactor", float, outMetallicRoughness->Metallic);
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jPbrMetallicRoughness, "roughnessFactor", float, outMetallicRoughness->Roughness);
        
        ParseVec4(jPbrMetallicRoughness, "baseColorFactor", outMetallicRoughness->BaseColour);
        ParseTextureInfo(jPbrMetallicRoughness, "baseColorTexture", outMetallicRoughness->BaseColourTexture);
        
        ParseTextureInfo(jPbrMetallicRoughness, "metallicRoughnessTexture", outMetallicRoughness->MetallicRoughnessTexture);
    }
    
    void JsonParser::ParsePbrSpecularGlossiness(simdjson::fallback::ondemand::object& jPbrSpecularGlossiness, std::optional<KHRMaterialPbrSpecularGlossiness> &outSpecularGlossiness)
    {
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jPbrSpecularGlossiness, "glossinessFactor", float, outSpecularGlossiness->GlossinessFactor);
        ParseTextureInfo(jPbrSpecularGlossiness, "specularGlossinessTexture", outSpecularGlossiness->SpecularGlossinessTexture);
        
        ParseVec4(jPbrSpecularGlossiness, "diffuseFactor", outSpecularGlossiness->DiffuseFactor);
        ParseTextureInfo(jPbrSpecularGlossiness, "diffuseTexture", outSpecularGlossiness->DiffuseTexture);
        
        ParseVec3(jPbrSpecularGlossiness, "specularFactor", outSpecularGlossiness->SpecularFactor);
    }
    
    void JsonParser::ParseSpecular(simdjson::fallback::ondemand::object& jSpecular, std::optional<KHRMaterialSpecular> &outSpecular)
    {
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jSpecular, "specularFactor", float, outSpecular->SpecularFactor);
        ParseTextureInfo(jSpecular, "specularTexture", outSpecular->SpecularTexture);
        
        ParseVec3(jSpecular, "specularColorFactor", outSpecular->SpecularColourFactor);
        ParseTextureInfo(jSpecular, "specularColorTexture", outSpecular->SpecularColourTexture);
        
    }
    
    void JsonParser::ParseClearcoat(simdjson::fallback::ondemand::object& jClearCoat, std::optional<KHRMaterialClearcoat>& outClearcoat)
    {
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jClearCoat, "clearcoatFactor", float, outClearcoat->ClearcoatFactor);
        ParseTextureInfo(jClearCoat, "clearcoatTexture", outClearcoat->ClearcoatTexture);
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jClearCoat, "clearcoatRoughnessFactor", float, outClearcoat->ClearcoatRoughnessFactor);
        ParseTextureInfo(jClearCoat, "clearcoatRoughnessTexture", outClearcoat->ClearcoatRoughnessTexture);
        
        ParseTextureInfo(jClearCoat, "clearcoatNormalTexture", outClearcoat->ClearcoatNormalTexture);

    }
    
    void JsonParser::ParseSheen(simdjson::fallback::ondemand::object& jSheen, std::optional<KHRMaterialSheen>& outSheen)
    {
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jSheen, "sheenRoughnessFactor", float, outSheen->SheenRoughnessFactor);
        ParseTextureInfo(jSheen, "sheenRoughnessTexture", outSheen->SheenRoughnessTexture);
        
        ParseVec3(jSheen, "sheenColorFactor", outSheen->SheenColourFactor);
        ParseTextureInfo(jSheen, "sheenColourTexture", outSheen->SheenColourTexture);
        
    }
    
    void JsonParser::ParseTransmission(simdjson::fallback::ondemand::object& jTransmission, std::optional<KHRMaterialTransmission>& outTransmission)
    {
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jTransmission, "transmissionFactor", float, outTransmission->TransmissionFactor);
        ParseTextureInfo(jTransmission, "transmissionTexture", outTransmission->TransmissionTexture);

    }
    
    void JsonParser::ParseVolume(simdjson::fallback::ondemand::object& jVolume, std::optional<KHRMaterialVolume>& outVolume)
    {
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jVolume, "thicknessFactor", float, outVolume->ThicknessFactor);
        ParseTextureInfo(jVolume, "thicknessTexture", outVolume->ThicknessTexture);
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jVolume, "attenuationDistance", float, outVolume->AttenuationDistance);
        ParseVec3(jVolume, "attenuationColor", outVolume->AttenuationColour);
    }
    
    void JsonParser::ParseIOR(simdjson::fallback::ondemand::object& jIOR, std::optional<KHRMaterialIOR>& outIOR)
    {
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jIOR, "ior", float, outIOR->IOR);
    }
    
    void JsonParser::ParseEmissiveStrength(simdjson::fallback::ondemand::object& jEmissiveStrength, std::optional<KHRMaterialEmissiveStrength>& outEmissiveStrength)
    {
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jEmissiveStrength, "emissiveStrength", float, outEmissiveStrength->EmissiveStrength);
    }
    
    void JsonParser::ParseIridescence(simdjson::fallback::ondemand::object& jIridescence, std::optional<KHRMaterialIridescence>& outIridescence)
    {
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jIridescence, "iridescenceFactor", float, outIridescence->IridescenceFactor);
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jIridescence, "iridescenceIor", float, outIridescence->IridescenceIor);
        ParseTextureInfo(jIridescence, "iridescenceTexture", outIridescence->IridescenceTexture);
        
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jIridescence, "iridescenceThicknessMinimum", float, outIridescence->IridescenceThicknessMinimum);
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jIridescence, "iridescenceThicknessMaximum", float, outIridescence->IridescenceThicknessMaximum);
        ParseTextureInfo(jIridescence, "iridescenceThicknessTexture", outIridescence->IridescenceThicknessTexture);
    }
    
    void JsonParser::ParseAnisotropy(simdjson::fallback::ondemand::object& jAnisotropy, std::optional<KHRMaterialAnisotropy>& outAnisotropy)
    {
        OCASI_SET_PROPERTY_IF_EXISTS_TEMPLATE(jAnisotropy, "anisotropyFactor", float, outAnisotropy->AnisotropyFactor);
        ParseVec3(jAnisotropy, "anisotropyDirection", outAnisotropy->AnisotropyDirection);
        ParseTextureInfo(jAnisotropy, "anisotropyTexture", outAnisotropy->AnisotropyTexture);
    }

    void JsonParser::ParseVertexAttributes(simdjson::fallback::ondemand::object& jVertexAttributes, VertexAttributes& outAttributes)
    {
        size_t vertexAttributeCount;
        
        for (auto jAttribute : jVertexAttributes)
        {
            std::string_view key;
            OCASI_FAIL_ON_SIMDJSON_ERROR(jAttribute.escaped_key().get(key), "Failed to get vertex jAttribute key");
            OCASI_FAIL_ON_SIMDJSON_ERROR(jAttribute.value().get(outAttributes[std::string(key)]), "Failed to get vertex jAttribute value");
        }
    }
    
    void JsonParser::ParseVec3(simdjson::fallback::ondemand::object& jObject, std::string_view name, glm::vec3& out)
    {
        ondemand::array jVec;
        OCASI_HAS_PROPERTY(jObject, name, jVec)
        {
            size_t i = 0;
            for (auto jVecVal : jVec) {
                
                float element;
                OCASI_FAIL_ON_SIMDJSON_ERROR(jVecVal.get<float>().get(element), "Failed to parse vec3 array element");
                
                out[(int)i] = element;
                
                i++;
            }
            OCASI_ASSERT(i == 3);
        }
    }
    
    void JsonParser::ParseVec4(simdjson::fallback::ondemand::object& jObject, std::string_view name, glm::vec4& out)
    {
        ondemand::array jVec;
        OCASI_HAS_PROPERTY(jObject, name, jVec)
        {
            size_t i = 0;
            for (auto jVecVal : jVec) {
                
                float element;
                OCASI_FAIL_ON_SIMDJSON_ERROR(jVecVal.get<float>().get(element), "Failed to parse vec4 array element");
                
                out[(int)i] = element;
                
                i++;
            }
            OCASI_ASSERT(i == 4);
        }
    }
}