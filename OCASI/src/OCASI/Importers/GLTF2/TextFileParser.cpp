#include "TextFileParser.h"

#include "OCASI/Importers/GLTF2/Json.h"

namespace OCASI::GLTF {

    const std::string EXTENSIONS_USED_PROPERTY = "extensionsUsed";
    const std::string EXTENSIONS_REQUIRED_PROPERTY = "extensionsRequired";
    const std::string ACCESSORS_PROPERTY = "accessors";
    const std::string BUFFER_VIEWS_PROPERTY = "bufferViews";
    const std::string BUFFERS_PROPERTY = "buffers";
    const std::string NODES_PROPERTY = "nodes";
    const std::string ASSET_PROPERTY = "asset";
    const std::string MESHES_PROPERTY = "meshes";
    const std::string MATERIALS_PROPERTY = "materials";
    const std::string TEXTURES_PROPERTY = "textures";
    const std::string IMAGES_PROPERTY = "images";
    const std::string SAMPLERS_PROPERTY = "samplers";
    const std::string SCENE_PROPERTY = "scene";
    const std::string SCENES_PROPERTY = "scenes";

    TextFileParser::TextFileParser(FileReader &reader)
        : m_FileReader(reader)
    {}

    TextFileParser::~TextFileParser()
    {
        delete m_Json;
    }

    std::shared_ptr<Asset> TextFileParser::ParseGLTFTextFile()
    {
        m_Json = new Json;
        if (auto error = glz::read_json(m_Json->Json, m_FileReader.GetFileString()))
        {
            OCASI_FAIL(FORMAT("Failed to load json: {}", error.custom_error_message));
            return nullptr;
        }

        m_Asset = std::make_shared<Asset>();
        glz::json_t& json = m_Json->Json;

        // The order of how things are imported doesn't really matter, however it does make sense
        // to first read in asset and extensions, followed by all data objects and ending with
        // meshes nodes and scenes, in order to simulate a form of importing were the different parts
        // would rely on each other.

        // Read the projects generator and version
        if(!ParseAssetDescription())
            return nullptr;

        // Read in all used extensions and check for their support
        if(!ParseExtensions())
            return nullptr;

        if (json.contains(SCENE_PROPERTY))
        {
            m_Asset->DefaultSceneIndex = (size_t) json.at(SCENE_PROPERTY).get_number();
        }

        if(!ParseBuffers())
            return nullptr;

        if(!ParseBufferViews())
            return nullptr;

        if(!ParseAccessors())
            return nullptr;

        ParseImages();
        ParseSamplers();
        ParseTextures();

        if(!ParseMaterials())
            return nullptr;

        if(!ParseMeshes())
            return nullptr;

        if(!ParseNodes())
            return nullptr;

        if(!ParseScenes())
            return nullptr;

        return m_Asset;
    }

    bool TextFileParser::ParseAssetDescription()
    {
        glz::json_t& json = m_Json->Json;

        if (!json.contains(ASSET_PROPERTY))
        {
            OCASI_FAIL("Required 'asset' object is not present in glTF file.");
            return false;
        }

        glz::json_t& asset = json.at(ASSET_PROPERTY);

        // The asset files version
        if (!asset.contains("version"))
        {
            OCASI_FAIL("Required 'version' property in asset is not present, though mandatory.");
            return false;
        }

        m_Asset->AssetVersion = {};
        std::string& versionString = asset.at("version").get_string();
        m_Asset->AssetVersion.Major = std::stoi(versionString.substr(0, versionString.find('.')));
        m_Asset->AssetVersion.Minor = std::stoi(versionString.substr(versionString.find('.')));
        OCASI_ASSERT(m_Asset->AssetVersion.Major == 2);

        if (asset.contains("minVersion"))
        {
            m_Asset->MinimumRequiredVersion = {};
            std::string& minVersionString = asset.at("minVersion").get_string();
            m_Asset->MinimumRequiredVersion->Major = std::stoi(minVersionString.substr(0, minVersionString.find('.')));
            m_Asset->MinimumRequiredVersion->Minor = std::stoi(minVersionString.substr(minVersionString.find('.')));
        }

        if (asset.contains("generator"))
        {
            m_Asset->Generator = asset.at("generator").get_string();
        }

        if (asset.contains("copyright"))
        {
            m_Asset->CopyRight = asset.at("copyright").get_string();
        }

        return true;
    }

    bool TextFileParser::ParseExtensions() {
        glz::json_t& json = m_Json->Json;

        // Return when there are no extensions required
        if (!json.contains(EXTENSIONS_USED_PROPERTY))
            return true;

        OCASI_ASSERT(json.at(EXTENSIONS_USED_PROPERTY).is_array());
        auto& extensions = json.at(EXTENSIONS_USED_PROPERTY).get_array();

        for (auto& ext : extensions)
        {
            std::string& extName = ext.get_string();
            m_Asset->ExtensionsUsed.push_back(extName);

            if (std::find(SUPPORTED_EXTENSIONS.begin(), SUPPORTED_EXTENSIONS.end(), extName) == SUPPORTED_EXTENSIONS.end())
                m_Asset->SupportedExtensionsUsed.push_back(extName);

        }

        // OCASI currently does not support any extensions that are required, so we return when there is an 'extensionRequired' section
        if (json.contains(EXTENSIONS_REQUIRED_PROPERTY))
        {
            OCASI_FAIL("Can't load glTF 2.0 file because it requires an unsupported extensions.");
            return false;
        }

        return true;
    }

    bool TextFileParser::ParseBuffers()
    {
        glz::json_t& json = m_Json->Json;

        if (!json.contains(BUFFERS_PROPERTY))
            return true;

        OCASI_ASSERT(json.at(BUFFERS_PROPERTY).is_array());
        auto& buffers = json.at(BUFFERS_PROPERTY).get_array();

        for (int i = 0; i < buffers.size(); i++)
        {
            glz::json_t& buffer = buffers.at(i);

            if (!buffer.contains("byteLength"))
            {
                OCASI_FAIL("Required 'byteLength' property in buffer is not present, though mandatory.");
                return false;
            }

            std::string& data = buffer.get_string();
            if (data.starts_with("data:"))
            {
                std::string uri = data.substr(data.find(':'));
                m_Asset->Buffers.emplace_back(i, uri, (size_t) buffer.at("byteLength").get_number());
            }
            else
            {
                FileReader binFileReader(m_FileReader.GetParentPath() / data, true);
                m_Asset->Buffers.emplace_back(i, binFileReader, (size_t) buffer.at("byteLength").get_number());
            }
        }

        return true;
    }

    bool TextFileParser::ParseBufferViews()
    {
        glz::json_t& json = m_Json->Json;

        if (!json.contains(BUFFER_VIEWS_PROPERTY))
            return true;

        OCASI_ASSERT(json.at(BUFFER_VIEWS_PROPERTY).is_array());
        auto& bufferViews = json.at(BUFFER_VIEWS_PROPERTY).get_array();

        for (int i = 0; i < bufferViews.size(); i++)
        {
            glz::json_t& jBufferView = bufferViews.at(i);

            if (!jBufferView.contains("buffer"))
            {
                OCASI_FAIL("Required 'buffer' property in bufferView is not present, though mandatory.");
                return false;
            }

            if (!jBufferView.contains("byteLength"))
            {
                OCASI_FAIL("Required 'byteLength' property in bufferView is not present, though mandatory.");
                return false;
            }

            BufferView& bufferView = m_Asset->BufferViews.emplace_back(i);
            bufferView.Buffer = (size_t) jBufferView.at("buffer").get_number();
            bufferView.ByteLength = (size_t) jBufferView.at("bufferView").get_number();

            if (jBufferView.contains("byteOffset"))
                bufferView.ByteOffset = (size_t) jBufferView.at("bufferView").get_number();

            if (jBufferView.contains("byteStride"))
                bufferView.ByteStride = (size_t) jBufferView.at("byteStride").get_number();
        }

        return true;
    }

    bool TextFileParser::ParseAccessors() {
        glz::json_t& json = m_Json->Json;

        if (!json.contains(ACCESSORS_PROPERTY))
            return true;

        OCASI_ASSERT(json.at(ACCESSORS_PROPERTY).is_array());
        auto& accessors = json.at(ACCESSORS_PROPERTY).get_array();

        for (int i = 0; i < accessors.size(); i++)
        {
            glz::json_t& jAccessor = accessors.at(i);

            if (!jAccessor.contains("componentType"))
            {
                OCASI_FAIL("Required 'componentType' property in accessor is not present, though mandatory.");
                return false;
            }

            if (!jAccessor.contains("count"))
            {
                OCASI_FAIL("Required 'byteLength' property in accessor is not present, though mandatory.");
                return false;
            }

            if (!jAccessor.contains("type"))
            {
                OCASI_FAIL("Required 'type' property in accessor is not present, though mandatory.");
                return false;
            }

            Accessor& accessor = m_Asset->Accessors.emplace_back(i);
            accessor.ComponentType = (ComponentType) jAccessor.at("componentType").get_number();
            accessor.ElementCount = (size_t) jAccessor.at("count").get_number();
            accessor.ComponentType = (ComponentType) jAccessor.at("type").get_number();

            if (jAccessor.contains("bufferView"))
                accessor.BufferView = (size_t) jAccessor.at("bufferView").get_number();

            if (jAccessor.contains("byteOffset"))
                accessor.ByteOffset = (size_t) jAccessor.at("byteOffset").get_number();

            if (jAccessor.contains("normalized"))
                accessor.Normalized = (size_t) jAccessor.at("normalized").get_number();

            if (jAccessor.contains("max"))
            {
                OCASI_ASSERT(jAccessor.at("max").is_array());
                auto& max = jAccessor.at("max").get_array();

                for (int j = 0; j < max.size(); j++)
                    accessor.MaxValues[j] = max.at(j).get_number();
            }

            if (jAccessor.contains("min"))
            {
                OCASI_ASSERT(jAccessor.at("min").is_array());
                auto& min = jAccessor.at("min").get_array();

                for (int j = 0; j < min.size(); j++)
                    accessor.MinValues[j] = min.at(j).get_number();
            }

            if (jAccessor.contains("sparse"))
            {
                Json j = { jAccessor };
                if (!ParseSparseAccessor(&j, accessor.Sparse))
                    return false;
            }
        }

        return true;
    }

    void TextFileParser::ParseImages()
    {
        glz::json_t& json = m_Json->Json;

        if (!json.contains(IMAGES_PROPERTY))
            return;

        OCASI_ASSERT(json.at(IMAGES_PROPERTY).is_array());
        auto& images = json.at(IMAGES_PROPERTY).get_array();

        for (int i = 0; i < images.size(); i++)
        {
            glz::json_t& jImage = images.at(i);
            Image& image = m_Asset->Images.emplace_back(i);

            if (jImage.contains("uri"))
                image.URI = jImage.at("uri").get_string();

            if (jImage.contains("mimeType"))
                image.MimeType = jImage.at("mimeType").get_string();

            if (jImage.contains("bufferView"))
                image.BufferView = (size_t) jImage.at("bufferView").get_number();
        }
    }

    void TextFileParser::ParseSamplers()
    {
        glz::json_t& json = m_Json->Json;

        if (!json.contains(SAMPLERS_PROPERTY))
            return;

        OCASI_ASSERT(json.at(SAMPLERS_PROPERTY).is_array());
        auto& samplers = json.at(SAMPLERS_PROPERTY).get_array();

        for (int i = 0; i < samplers.size(); i++)
        {
            glz::json_t& jSampler = samplers.at(i);
            Sampler& sampler = m_Asset->Samplers.emplace_back(i);

            if (jSampler.contains("magFilter"))
                sampler.MagFilter = (MinMagFilter) jSampler.at("magFilter").get_number();

            if (jSampler.contains("minFilter"))
                sampler.MagFilter = (MinMagFilter) jSampler.at("magFilter").get_number();

            if (jSampler.contains("wrapS"))
                sampler.WrapS = (UVWrap) jSampler.at("wrapS").get_number();

            if (jSampler.contains("wrapT"))
                sampler.WrapT = (UVWrap) jSampler.at("wrapT").get_number();
        }
    }

    bool TextFileParser::ParseSparseAccessor(const Json* jsonAccessor, std::optional<Sparse>& outSparse)
    {
        const glz::json_t& jSparse = jsonAccessor->Json.at("sparse");

        if (!jSparse.contains("count"))
        {
            OCASI_FAIL("Required 'count' property in sparse accessor is not present, though mandatory.");
            return false;
        }

        if (!jSparse.contains("indices"))
        {
            OCASI_FAIL("Required 'indices' property in sparse accessor is not present, though mandatory.");
            return false;
        }

        if (!jSparse.contains("values"))
        {
            OCASI_FAIL("Required 'values' property in sparse accessor is not present, though mandatory.");
            return false;
        }

        outSparse = {};
        outSparse->ElementCount = (size_t) jSparse.at("count").get_number();

        // Indices
        {
            const glz::json_t& jSparseIndices = jSparse.at("indices");

            if (!jSparseIndices.contains("bufferView"))
            {
                OCASI_FAIL("Required 'bufferView' property in sparse accessor indices is not present, though mandatory.");
                return false;
            }

            if (!jSparseIndices.contains("componentType"))
            {
                OCASI_FAIL("Required 'componentType' property in sparse accessor indices is not present, though mandatory.");
                return false;
            }

            SparseIndices& indices = outSparse->Indices;
            indices.BufferView = (size_t) jSparseIndices.at("bufferView").get_number();
            indices.componentType = (ComponentType) jSparseIndices.at("componentType").get_number();

            if (jSparseIndices.contains("byteOffset"))
                indices.BufferView = (size_t) jSparseIndices.at("byteOffset").get_number();
        }

        // Values
        {
            const glz::json_t& jSparseValues = jSparse.at("values");

            if (!jSparseValues.contains("bufferView"))
            {
                OCASI_FAIL("Required 'bufferView' property in sparse accessor indices is not present, though mandatory.");
                return false;
            }

            SparseValues& values = outSparse->Values;
            values.BufferView = (size_t) jSparseValues.at("bufferView").get_number();
            values.ByteOffset = (size_t) jSparseValues.at("byteOffset").get_number();
        }

        return true;
    }

    void TextFileParser::ParseTextures()
    {
        glz::json_t& json = m_Json->Json;

        if (!json.contains(TEXTURES_PROPERTY))
            return;

        OCASI_ASSERT(json.at(TEXTURES_PROPERTY).is_array());
        auto& textures = json.at(TEXTURES_PROPERTY).get_array();

        for (int i = 0; i < textures.size(); i++)
        {
            glz::json_t& jTexture = textures.at(i);
            Texture& texture = m_Asset->Textures.emplace_back(i);

            if (jTexture.contains("source"))
                texture.Source = (size_t) jTexture.at("source").get_number();

            if (jTexture.contains("sampler"))
                texture.Sampler = (size_t) jTexture.at("sampler").get_number();
        }
    }

    bool TextFileParser::ParseMaterials()
    {
        glz::json_t& json = m_Json->Json;

        if (!json.contains(MATERIALS_PROPERTY))
            return true;

        OCASI_ASSERT(json.at(MATERIALS_PROPERTY).is_array());
        auto& materials = json.at(MATERIALS_PROPERTY).get_array();

        for (int i = 0; i < materials.size(); i++)
        {
            glz::json_t& jMaterial = materials.at(i);
            Material& material = m_Asset->Materials.emplace_back(i);

            if (jMaterial.contains("name"))
                material.Name = jMaterial.at("name").get_string();

            if (jMaterial.contains("pbrMetallicRoughness"))
            {
                Json jsonPbr = { jMaterial.at("pbrMetallicRoughness") };
                if (!ParsePbrMetallicRoughness(&jsonPbr, material.MetallicRoughness))
                    return false;
            }

            if (jMaterial.contains("normalTexture"))
            {
                Json jsonNormal = { jMaterial.at("normalTexture") };
                if (!ParseTextureInfo(&jsonNormal, material.NormalTexture))
                    return false;
            }

            if (jMaterial.contains("occlusionTexture"))
            {
                Json jsonOcclusion = { jMaterial.at("occlusionTexture") };
                if (!ParseTextureInfo(&jsonOcclusion, material.OcclusionTexture))
                    return false;
            }

            if (jMaterial.contains("emissiveTexture"))
            {
                Json jsonEmissive = { jMaterial.at("emissiveTexture") };
                if (!ParseTextureInfo(&jsonEmissive, material.EmissiveTexture))
                    return false;
            }

            if (jMaterial.contains("emissiveFactor"))
            {
                auto& jEmissiveFactor = jMaterial.at("emissiveFactor").get_array();

                material.EmissiveColour = glm::vec3(
                        (float) jEmissiveFactor.at(0).get_number(),
                        (float) jEmissiveFactor.at(1).get_number(),
                        (float) jEmissiveFactor.at(2).get_number());
            }

            if (jMaterial.contains("alphaMode"))
            {
                std::string& jAlphaMode = jMaterial.at("alphaMode").get_string();

                if (jAlphaMode == "OPAQUE")
                    material.AlphaMode = AlphaMode::Opaque;
                else if (jAlphaMode == "MASK")
                    material.AlphaMode = AlphaMode::Mask;
                else if (jAlphaMode == "BLEND")
                    material.AlphaMode = AlphaMode::Blend;
                else
                {
                    OCASI_FAIL(FORMAT("Invalid value for alphaMode in material of index {}. Value: {}", i, jAlphaMode))
                }
            }

            if (jMaterial.contains("alphaCutoff"))
                material.AlphaCutoff = (float) jMaterial.at("alphaCutoff").get_number();

            if (jMaterial.contains("doubleSided"))
                material.IsDoubleSided = jMaterial.at("doubleSided").get_boolean();

            /// Extensions

        }

        return true;
    }

    bool TextFileParser::ParsePbrMetallicRoughness(const Json *jsonPbrMetallicRoughness, std::optional<PBRMetallicRoughness> &outMetallicRoughness)
    {
        const glz::json_t& jPbrMetallicRoughness = jsonPbrMetallicRoughness->Json;

        if (jPbrMetallicRoughness.contains("baseColorFactor"))
        {
            const auto& jBaseColorFactor = jPbrMetallicRoughness.at("baseColorFactor").get_array();

            OCASI_ASSERT(jBaseColorFactor.size() == 4);

            // TODO: Consider using 64 floating point number
            outMetallicRoughness->BaseColour = glm::vec4(
                    (float) jBaseColorFactor.at(0).get_number(),
                    (float) jBaseColorFactor.at(1).get_number(),
                    (float) jBaseColorFactor.at(2).get_number(),
                    (float) jBaseColorFactor.at(3).get_number());
        }

        if (jPbrMetallicRoughness.contains("baseColorTexture"))
        {
            Json json = { jPbrMetallicRoughness.at("baseColorTexture") };
            if (!ParseTextureInfo(&json, outMetallicRoughness->BaseColourTexture))
                return false;
        }

        if (jPbrMetallicRoughness.contains("metallicFactor"))
            outMetallicRoughness->Metallic = (float) jPbrMetallicRoughness.at("metallicFactor").get_number();

        if (jPbrMetallicRoughness.contains("roughnessFactor"))
            outMetallicRoughness->Roughness = (float) jPbrMetallicRoughness.at("roughnessFactor").get_number();

        if (jPbrMetallicRoughness.contains("metallicRoughnessTexture"))
        {
            Json json = { jPbrMetallicRoughness.at("metallicRoughnessTexture") };
            if (!ParseTextureInfo(&json, outMetallicRoughness->MetallicRoughnessTexture))
                return false;
        }

        return true;
    }

    bool TextFileParser::ParseTextureInfo(const Json *jsonTextureInfo, std::optional<TextureInfo>& outTextureInfo)
    {
        const glz::json_t& jPbrMetallicRoughness = jsonTextureInfo->Json;

        if (!jPbrMetallicRoughness.contains("index"))
        {
            OCASI_FAIL("Required 'index' property in texture info accessor indices is not present, though mandatory.");
            return false;
        }

        outTextureInfo = {};
        outTextureInfo->Texture = (size_t) jPbrMetallicRoughness.at("index").get_number();

        if (jPbrMetallicRoughness.contains("texCoord"))
            outTextureInfo->TexCoords = (size_t) jPbrMetallicRoughness.at("texCoord").get_number();

        if (jPbrMetallicRoughness.contains("scale"))
            outTextureInfo->Scale = (float) jPbrMetallicRoughness.at("scale").get_number();

        if (jPbrMetallicRoughness.contains("strength"))
            outTextureInfo->Scale = (float) jPbrMetallicRoughness.at("strength").get_number();

        return true;
    }

    bool TextFileParser::ParsePbrSpecularGlossiness(const Json* jsonExtension, std::optional<KHRMaterialPbrSpecularGlossiness> &outMetallicRoughness)
    {
        const glz::json_t& jPbrSpecularGlossiness = jsonExtension->Json.at("KHR_materials_pbrSpecularGlossiness");

        if (jPbrSpecularGlossiness.contains("diffuseFactor"))
        {
            outMetallicRoughness->DiffuseFactor = glm::vec4(
                    jPbrSpecularGlossiness["diffuseFactor"][0].get_number(), jPbrSpecularGlossiness["diffuseFactor"][1].get_number(),
                    jPbrSpecularGlossiness["diffuseFactor"][2].get_number(), jPbrSpecularGlossiness["diffuseFactor"][3].get_number()
            );
        }

        if (jPbrSpecularGlossiness.contains("diffuseTexture"))
        {
            Json json = { jPbrSpecularGlossiness.at("diffuseTexture") };
            if (!ParseTextureInfo(&json, outMetallicRoughness->DiffuseTexture))
                return false;
        }

        if (jPbrSpecularGlossiness.contains("specularFactor"))
        {
            outMetallicRoughness->SpecularFactor = glm::vec3(
                    jPbrSpecularGlossiness["specularFactor"][0].get_number(), jPbrSpecularGlossiness["specularFactor"][1].get_number(),
                    jPbrSpecularGlossiness["specularFactor"][2].get_number()
            );
        }

        if (jPbrSpecularGlossiness.contains("specularGlossinessTexture"))
        {
            Json json = { jPbrSpecularGlossiness.at("specularGlossinessTexture") };
            if (!ParseTextureInfo(&json, outMetallicRoughness->SpecularGlossinessTexture))
                return false;
        }

        if (jPbrSpecularGlossiness.contains("glossinessFactor"))
            outMetallicRoughness->GlossinessFactor = (float) jPbrSpecularGlossiness["glossinessFactor"].get_number();

        return true;
    }

    bool TextFileParser::ParseClearcoat(const Json* jsonExtension, std::optional<KHRMaterialClearcoat>& outClearcoat)
    {
        const glz::json_t& jClearcoat = jsonExtension->Json.at("KHR_materials_clearcoat");

        if (jClearcoat.contains("clearcoatFactor"))
            outClearcoat->ClearcoatFactor = (float) jClearcoat["clearcoatFactor"].get_number();

        if (jClearcoat.contains("clearcoatRoughnessFactor"))
            outClearcoat->ClearcoatRoughnessFactor = (float) jClearcoat["clearcoatRoughnessFactor"].get_number();

        if (jClearcoat.contains("clearcoatTexture"))
        {
            Json json = { jClearcoat.at("clearcoatTexture") };
            if (!ParseTextureInfo(&json, outClearcoat->ClearcoatTexture))
                return false;
        }

        if (jClearcoat.contains("clearcoatRoughnessTexture"))
        {
            Json json = { jClearcoat.at("clearcoatRoughnessTexture") };
            if (!ParseTextureInfo(&json, outClearcoat->ClearcoatRoughnessTexture))
                return false;
        }

        if (jClearcoat.contains("clearcoatNormalTexture"))
        {
            Json json = { jClearcoat.at("clearcoatNormalTexture") };
            if (!ParseTextureInfo(&json, outClearcoat->ClearcoatNormalTexture))
                return false;
        }

        return true;
    }

    bool TextFileParser::ParseSheen(const Json *jsonExtension, std::optional<KHRMaterialSheen>& outSheen)
    {
        const glz::json_t& jSheen = jsonExtension->Json.at("KHR_materials_sheen");

        if (jSheen.contains("sheenColourTexture"))
        {
            Json json = { jSheen.at("sheenColourTexture") };
            if (!ParseTextureInfo(&json, outSheen->SheenColourTexture))
                return false;
        }

        if (jSheen.contains("sheenRoughnessTexture"))
        {
            Json json = { jSheen.at("sheenRoughnessTexture") };
            if (!ParseTextureInfo(&json, outSheen->SheenRoughnessTexture))
                return false;

        }

        if (jSheen.contains("sheenColourFactor"))
        {
            auto& jSheenColourFactor = jSheen.at("sheenColourFactor").get_array();

            outSheen->SheenColourFactor = glm::vec3(
                    jSheenColourFactor.at(0).get_number(),
                    jSheenColourFactor.at(1).get_number(),
                    jSheenColourFactor.at(2).get_number());
        }

        if (jSheen.contains("sheenRoughnessFactor"))
            outSheen->SheenRoughnessFactor = (float) jSheen.at("sheenRoughnessFactor").get_number();

        return true;
    }

    bool TextFileParser::ParseTransmission(const Json *jsonExtension, std::optional<KHRMaterialTransmission>& outTransmission)
    {
        const glz::json_t& jTransmission = jsonExtension->Json.at("KHR_materials_transmission");

        if (jTransmission.contains("transmissionFactor"))
            outTransmission->TransmissionFactor = (float) jTransmission.at("transmissionFactor").get_number();

        if (jTransmission.contains("transmissionTexture"))
        {
            Json json = { jTransmission.at("transmissionTexture") };
            if (!ParseTextureInfo(&json, outTransmission->TransmissionTexture))
                return false;
        }

        return true;
    }

    bool TextFileParser::ParseVolume(const Json *jsonExtension, std::optional<KHRMaterialVolume>& outVolume)
    {
        const glz::json_t& jVolume = jsonExtension->Json.at("KHR_materials_volume");

        if (jVolume.contains("thicknessFactor"))
            outVolume->ThicknessFactor = (float) jVolume.at("thicknessFactor").get_number();


        if (jVolume.contains("thicknessTexture"))
        {
            Json json = { jVolume.at("thicknessTexture") };
            if (!ParseTextureInfo(&json, outVolume->ThicknessTexture))
                return false;
        }

        if (jVolume.contains("attenuationDistance"))
            outVolume->AttenuationDistance = (float) jVolume["attenuationDistance"].get_number();

        if (jVolume.contains("attenuationColor"))
        {
            auto& jAttenuationColour = jVolume.at("attenuationColor").get_array();
            outVolume->AttenuationColour = glm::vec3(
                    jAttenuationColour.at(0).get_number(),
                    jAttenuationColour.at(1).get_number(),
                    jAttenuationColour.at(2).get_number());
        }

        return true;
    }

    void TextFileParser::ParseIOR(const Json *jsonExtension, std::optional<KHRMaterialIOR>& outIOR)
    {
        const glz::json_t& jIOR = jsonExtension->Json.at("KHR_materials_ior");

        if (jIOR.contains("ior")) {
            outIOR->Ior = (float) jIOR.at("ior").get_number();
        }
    }

    void TextFileParser::ParseEmissiveStrength(const Json *jsonExtension, std::optional<KHRMaterialEmissiveStrength>& outEmissiveStrength)
    {
        const glz::json_t& jEmissiveStrength = jsonExtension->Json.at("KHR_materials_emissive_strength");

        if (jEmissiveStrength.contains("emissiveStrength"))
            outEmissiveStrength->EmissiveStrength = (float) jEmissiveStrength.at("emissiveStrength").get_number();

    }

    bool TextFileParser::ParseIridescence(const Json *jsonExtension, std::optional<KHRMaterialIridescence>& outIridescence)
    {
        const glz::json_t& jIridescence = jsonExtension->Json.at("KHR_materials_iridescence");

        if (jIridescence.contains("iridescenceFactor"))
            outIridescence->IridescenceFactor = (float)jIridescence["iridescenceFactor"].get_number();

        if (jIridescence.contains("iridescenceTexture"))
        {
            Json json = { jIridescence.at("iridescenceTexture") };
            if (!ParseTextureInfo(&json, outIridescence->IridescenceTexture))
                return false;
        }

        if (jIridescence.contains("iridescenceIor"))
            outIridescence->IridescenceIor = (float)jIridescence["iridescenceIor"].get_number();

        if (jIridescence.contains("iridescenceThicknessMinimum"))
            outIridescence->IridescenceThicknessMinimum = (float)jIridescence["iridescenceThicknessMinimum"].get_number();

        if (jIridescence.contains("iridescenceThicknessMaximum"))
            outIridescence->IridescenceThicknessMaximum = (float)jIridescence["iridescenceThicknessMaximum"].get_number();


        if (jIridescence.contains("iridescenceThicknessTexture"))
        {
            Json json = { jIridescence.at("iridescenceThicknessTexture") };
            if (!ParseTextureInfo(&json, outIridescence->IridescenceThicknessTexture))
                return false;
        }

        return true;
    }

    bool TextFileParser::ParseAnisotropy(const Json *jsonExtension, std::optional<KHRMaterialAnisotropy>& outAnisotropy)
    {
        const glz::json_t& jAnisotropy = jsonExtension->Json.at("KHR_materials_anisotropy");

        if (jAnisotropy.contains("anisotropyFactor"))
            outAnisotropy->AnisotropyFactor = (float)jAnisotropy["anisotropyFactor"].get_number();

        if (jAnisotropy.contains("anisotropyTexture"))
        {
            Json json = { jAnisotropy.at("anisotropyTexture") };
            if (!ParseTextureInfo(&json, outAnisotropy->AnisotropyTexture))
                return false;
        }

        if (jAnisotropy.contains("anisotropyDirection"))
        {
            auto& jAnisotropyDirection = jAnisotropy.at("anisotropyDirection").get_array();
            outAnisotropy->AnisotropyDirection = glm::vec3(
                    jAnisotropyDirection.at(0).get_number(),
                    jAnisotropyDirection.at(1).get_number(),
                    jAnisotropyDirection.at(2).get_number()
            );
        }

        return true;
    }
}