#include "JsonParser.h"

#include "glaze/glaze.hpp"

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

    JsonParser::JsonParser(FileReader &reader)
        : m_FileReader(reader)
    {}

    JsonParser::JsonParser(FileReader &reader, glz::json_t* json)
        : m_FileReader(reader), m_Json(json)
    {}

    JsonParser::~JsonParser()
    {
        delete m_Json;
    }

    std::shared_ptr<Asset> JsonParser::ParseGLTFTextFile()
    {
        if (!m_Json)
        {
            m_Json = new glz::json_t;
            if (auto error = glz::read_json(m_Json, m_FileReader.GetFileString()); error)
            {
                OCASI_FAIL(FORMAT("Failed to load json: {}", error.custom_error_message));
                return nullptr;
            }
        }

        m_Asset = std::make_shared<Asset>();
        glz::json_t& json = *m_Json;

        // The order of how things are parsed doesn't really matter, however it does make sense
        // to first read in asset and extensions, followed by all data objects and ending with
        // the connecting objects, like meshes nodes and scenes, in order to simulate scenario
        // where the order of things would be fundamental.

        // Read the projects generator and version
        if(!ParseAssetDescription())
            return nullptr;

        // Read in all used extensions and check for their support
        if(!ParseExtensions())
            return nullptr;

        if (json.contains(SCENE_PROPERTY))
            m_Asset->DefaultSceneIndex = (size_t) json.at(SCENE_PROPERTY).get_number();

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

        ParseNodes();

        if(!ParseScenes())
            return nullptr;

        return m_Asset;
    }

    bool JsonParser::ParseAssetDescription()
    {
        glz::json_t& json = *m_Json;

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
        m_Asset->AssetVersion.Minor = std::stoi(versionString.substr(versionString.find('.') + 1));
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

    bool JsonParser::ParseExtensions() {
        glz::json_t& json = *m_Json;

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

    bool JsonParser::ParseBuffers()
    {
        glz::json_t& json = *m_Json;

        if (!json.contains(BUFFERS_PROPERTY))
            return true;

        OCASI_ASSERT(json.at(BUFFERS_PROPERTY).is_array());
        auto& buffers = json.at(BUFFERS_PROPERTY).get_array();

        for (int i = 0; i < buffers.size(); i++)
        {
            glz::json_t& jBuffer = buffers.at(i);

            if (!jBuffer.contains("byteLength"))
            {
                OCASI_FAIL("Required 'byteLength' property in jBuffer is not present, though mandatory.");
                return false;
            }

            size_t byteLength = (size_t) jBuffer.at("byteLength").get_number();


            if (jBuffer.contains("uri"))
            {
                std::string& data = jBuffer.at("uri").get_string();
                if (data.starts_with("data:"))
                {
                    std::string uri = data.substr(data.find(':'));
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
        }

        return true;
    }

    bool JsonParser::ParseBufferViews()
    {
        glz::json_t& json = *m_Json;

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
            bufferView.ByteLength = (size_t) jBufferView.at("byteLength").get_number();

            if (jBufferView.contains("byteOffset"))
                bufferView.ByteOffset = (size_t) jBufferView.at("byteOffset").get_number();

            if (jBufferView.contains("byteStride"))
                bufferView.ByteStride = (size_t) jBufferView.at("byteStride").get_number();
        }

        return true;
    }

    bool JsonParser::ParseAccessors() {
        glz::json_t& json = *m_Json;

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

            // Reading the data type
            {
                std::string& dataType = jAccessor.at("type").get_string();

                if (dataType == "SCALAR")
                    accessor.DataType = DataType::Scalar;
                else if (dataType == "VEC2")
                    accessor.DataType = DataType::Vec2;
                else if (dataType == "VEC3")
                    accessor.DataType = DataType::Vec3;
                else if (dataType == "VEC4")
                    accessor.DataType = DataType::Vec4;
                else if (dataType == "MAT2")
                    accessor.DataType = DataType::Mat2;
                else if (dataType == "MAT3")
                    accessor.DataType = DataType::Mat3;
                else if (dataType == "MAT4")
                    accessor.DataType = DataType::Mat4;
                else
                {
                    OCASI_FAIL(FORMAT("The accessor data type string didn't match any case: {}", dataType));
                    return false;
                }
            }

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
                if (!ParseSparseAccessor(jAccessor.at("sparse"), accessor.Sparse = Sparse()))
                    return false;
            }
        }

        return true;
    }

    void JsonParser::ParseImages()
    {
        glz::json_t& json = *m_Json;

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

    void JsonParser::ParseSamplers()
    {
        glz::json_t& json = *m_Json;

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

    bool JsonParser::ParseSparseAccessor(const glz::json_t& jsonAccessor, std::optional<Sparse>& outSparse)
    {
        const glz::json_t& jSparse = *jsonAccessor;

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
            indices.ComponentType = (ComponentType) jSparseIndices.at("componentType").get_number();

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

    void JsonParser::ParseTextures()
    {
        glz::json_t& json = *m_Json;

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

    bool JsonParser::ParseMaterials()
    {
        glz::json_t& json = *m_Json;

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
                if (!ParsePbrMetallicRoughness(jMaterial.at("pbrMetallicRoughness"), material.MetallicRoughness = PBRMetallicRoughness()))
                    return false;
            }

            if (jMaterial.contains("normalTexture"))
            {
                if (!ParseTextureInfo(jMaterial.at("normalTexture"), material.NormalTexture = TextureInfo()))
                    return false;
            }

            if (jMaterial.contains("occlusionTexture"))
            {
                if (!ParseTextureInfo(jMaterial.at("occlusionTexture"), material.OcclusionTexture = TextureInfo()))
                    return false;
            }

            if (jMaterial.contains("emissiveTexture"))
            {
                if (!ParseTextureInfo(jMaterial.at("emissiveTexture"), material.EmissiveTexture = TextureInfo()))
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
            if (jMaterial.contains("extensions"))
            {
                glz::json_t& jExtensions = jMaterial.at("extensions");

                if (jExtensions.contains("KHR_materials_pbrSpecularGlossiness"))
                    ParsePbrSpecularGlossiness(jExtensions.at("KHR_materials_pbrSpecularGlossiness"), material.ExtSpecularGlossiness = KHRMaterialPbrSpecularGlossiness());

                if (jExtensions.contains("KHR_materials_specular"))
                    ParseSpecular(jExtensions.at("KHR_materials_specular"), material.ExtSpecular = KHRMaterialSpecular());

                if (jExtensions.contains("KHR_materials_clearcoat"))
                    ParseClearcoat(jExtensions.at("KHR_materials_clearcoat"), material.ExtClearcoat = KHRMaterialClearcoat());

                if (jExtensions.contains("KHR_materials_sheen"))
                    ParseSheen(jExtensions.at("KHR_materials_sheen"), material.ExtSheen = KHRMaterialSheen());

                if (jExtensions.contains("KHR_materials_transmission"))
                    ParseTransmission(jExtensions.at("KHR_materials_transmission"), material.ExtTransmission = KHRMaterialTransmission());

                if (jExtensions.contains("KHR_materials_volume"))
                    ParseVolume(jExtensions.at("KHR_materials_volume"), material.ExtVolume = KHRMaterialVolume());

                if (jExtensions.contains("KHR_materials_ior"))
                    ParseIOR(jExtensions.at("KHR_materials_ior"), material.ExtIOR = KHRMaterialIOR());

                if (jExtensions.contains("KHR_materials_emissive_strength"))
                    ParseEmissiveStrength(jExtensions.at("KHR_materials_emissive_strength"), material.ExtEmissiveStrength = KHRMaterialEmissiveStrength());

                if (jExtensions.contains("KHR_materials_iridescence"))
                    ParseIridescence(jExtensions.at("KHR_materials_iridescence"), material.ExtIridescence = KHRMaterialIridescence());

                if (jExtensions.contains("KHR_materials_anisotropy"))
                    ParseAnisotropy(jExtensions.at("KHR_materials_pbrSpecularGlossiness"), material.ExtAnisotropy = KHRMaterialAnisotropy());
            }
        }

        return true;
    }


    bool JsonParser::ParseMeshes()
    {
        glz::json_t& json = *m_Json;

        if (!json.contains(MESHES_PROPERTY))
            return true;

        OCASI_ASSERT(json.at(MESHES_PROPERTY).is_array());
        auto& meshes = json.at(MESHES_PROPERTY).get_array();

        for (int i = 0; i < meshes.size(); i++)
        {
            glz::json_t& jMesh = meshes.at(i);

            if (!jMesh.contains("primitives"))
            {
                OCASI_FAIL("Required 'primitives' property in mesh is not present, though mandatory.");
                return false;
            }

            Mesh& mesh = m_Asset->Meshes.emplace_back(i);
            if(!ParsePrimitives(jMesh.at("primitives"), mesh))
                return false;

            if (jMesh.contains("weights"))
            {
                auto& weights = jMesh.at("weights").get_array();
                mesh.Weights.resize(weights.size());

                for (int j = 0; j < weights.size(); j++)
                {
                    mesh.Weights[j] = (float) weights.at(j).get_number();
                }
            }
        }

        return true;
    }

    void JsonParser::ParseNodes()
    {
        glz::json_t& json = *m_Json;

        if (!json.contains(NODES_PROPERTY))
            return;

        OCASI_ASSERT(json.at(NODES_PROPERTY).is_array());
        auto& nodes = json.at(NODES_PROPERTY).get_array();

        for (int i = 0; i < nodes.size(); i++)
        {
            glz::json_t& jNode = nodes.at(i);
            Node& node = m_Asset->Nodes.emplace_back(i);

            // Cameras and animations are not supported

            if (jNode.contains("children"))
            {
                auto& children = jNode.at("children").get_array();
                node.Children.resize(children.size());
                for (int j = 0; j < children.size(); j++)
                {
                    node.Children[j] = (size_t) children.at(i).get_number();
                }
            }

            if (jNode.contains("name"))
                node.Name = jNode.at("name").get_string();

            if (jNode.contains("mesh"))
                node.Mesh = (size_t) jNode.at("mesh").get_number();

            if (jNode.contains("translation"))
                ParseVec3(jNode.at("translation"), node.TrsComponent->Translation);


            if (jNode.contains("rotation"))
            {
                glm::vec4 v;
                ParseVec4(jNode.at("rotation"), v);
                node.TrsComponent->Rotation = glm::quat(v);
            }

            if (jNode.contains("size"))
                ParseVec3(jNode.at("size"), node.TrsComponent->Scale);


            if (jNode.contains("matrix"))
            {
                auto& matrixValues = jNode.at("matrix").get_array();

                for (int j = 0; i < matrixValues.size(); j++)
                {
                    node.LocalTranslationMatrix.value()[j % 4][j / 4] = (float) matrixValues.at(j).get_number();
                }
            }

            if (jNode.contains("weights"))
            {
                auto& weights = jNode.at("weights").get_array();
                node.Weights.resize(weights.size());

                for (int j = 0; i < weights.size(); j++)
                {
                    node.Weights[j] = (float) weights.at(j).get_number();
                }
            }
        }
    }

    bool JsonParser::ParseScenes()
    {
        glz::json_t& json = *m_Json;

        if (!json.contains(SCENES_PROPERTY))
            return true;

        OCASI_ASSERT(json.at(SCENES_PROPERTY).is_array());
        auto& scenes = json.at(SCENES_PROPERTY).get_array();

        for (int i = 0; i < scenes.size(); i++)
        {
            glz::json_t& jScene = scenes.at(i);
            Scene& scene = m_Asset->Scenes.emplace_back(i);

            if (jScene.contains("name"))
                scene.Name = jScene.at("name").get_string();

            if (jScene.contains("nodes"))
            {
                auto& rootNodes = jScene.at("nodes").get_array();
                scene.RootNodes.resize(rootNodes.size());
                for (int j = 0; j < scenes.size(); j++)
                {
                    scene.RootNodes[j] = (size_t) rootNodes.at(j).get_number();
                }
            }
        }

        return true;
    }

    bool JsonParser::ParseTextureInfo(const glz::json_t& jTextureInfo, std::optional<TextureInfo>& outTextureInfo)
    {
        if (!jTextureInfo.contains("index"))
        {
            OCASI_FAIL("Required 'index' property in texture info is not present, though mandatory.");
            return false;
        }

        outTextureInfo->Texture = (size_t) jTextureInfo.at("index").get_number();

        if (jTextureInfo.contains("texCoord"))
            outTextureInfo->TexCoords = (size_t) jTextureInfo.at("texCoord").get_number();

        if (jTextureInfo.contains("scale"))
            outTextureInfo->Scale = (float) jTextureInfo.at("scale").get_number();

        if (jTextureInfo.contains("strength"))
            outTextureInfo->Scale = (float) jTextureInfo.at("strength").get_number();

        return true;
    }

    bool JsonParser::ParsePbrMetallicRoughness(const glz::json_t& jPbrMetallicRoughness, std::optional<PBRMetallicRoughness>& outMetallicRoughness)
    {
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
            if (!ParseTextureInfo(jPbrMetallicRoughness.at("baseColorTexture"), outMetallicRoughness->BaseColourTexture = TextureInfo()))
                return false;
        }

        if (jPbrMetallicRoughness.contains("metallicFactor"))
            outMetallicRoughness->Metallic = (float) jPbrMetallicRoughness.at("metallicFactor").get_number();

        if (jPbrMetallicRoughness.contains("roughnessFactor"))
            outMetallicRoughness->Roughness = (float) jPbrMetallicRoughness.at("roughnessFactor").get_number();

        if (jPbrMetallicRoughness.contains("metallicRoughnessTexture"))
        {
            if (!ParseTextureInfo(jPbrMetallicRoughness.at("metallicRoughnessTexture") , outMetallicRoughness->MetallicRoughnessTexture = TextureInfo()))
                return false;
        }

        return true;
    }

    bool JsonParser::ParsePbrSpecularGlossiness(const glz::json_t& jPbrSpecularGlossiness, std::optional<KHRMaterialPbrSpecularGlossiness> &outMetallicRoughness)
    {
        if (jPbrSpecularGlossiness.contains("diffuseFactor"))
        {
            outMetallicRoughness->DiffuseFactor = glm::vec4(
                    jPbrSpecularGlossiness["diffuseFactor"][0].get_number(), jPbrSpecularGlossiness["diffuseFactor"][1].get_number(),
                    jPbrSpecularGlossiness["diffuseFactor"][2].get_number(), jPbrSpecularGlossiness["diffuseFactor"][3].get_number()
            );
        }

        if (jPbrSpecularGlossiness.contains("diffuseTexture"))
        {
            if (!ParseTextureInfo(jPbrSpecularGlossiness.at("diffuseTexture"), outMetallicRoughness->DiffuseTexture = TextureInfo()))
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
            if (!ParseTextureInfo(jPbrSpecularGlossiness.at("specularGlossinessTexture"), outMetallicRoughness->SpecularGlossinessTexture = TextureInfo()))
                return false;
        }

        if (jPbrSpecularGlossiness.contains("glossinessFactor"))
            outMetallicRoughness->GlossinessFactor = (float) jPbrSpecularGlossiness["glossinessFactor"].get_number();

        return true;
    }

    bool JsonParser::ParseSpecular(const glz::json_t& jSpecular, std::optional<KHRMaterialSpecular> &outSpecular)
    {
        if (jSpecular.contains("specularFactor"))
            outSpecular->SpecularFactor = (float) jSpecular["specularFactor"].get_number();


        if (jSpecular.contains("specularTexture"))
        {
            if (!ParseTextureInfo(jSpecular.at("specularTexture"), outSpecular->SpecularTexture = TextureInfo()))
                return false;
        }

        if (jSpecular.contains("specularColorFactor"))
        {
            auto& jSpecularColorFactor = jSpecular.at("specularColorFactor").get_array();
            outSpecular->SpecularColourFactor = glm::vec3(
                    jSpecularColorFactor.at(0).get_number(),
                    jSpecularColorFactor.at(1).get_number(),
                    jSpecularColorFactor.at(2).get_number());
        }

        if (jSpecular.contains("specularColorTexture"))
        {
            if (!ParseTextureInfo(jSpecular.at("specularColorTexture"), outSpecular->SpecularColourTexture = TextureInfo()))
                return false;
        }

        return true;
    }

    bool JsonParser::ParseClearcoat(const glz::json_t& jClearcoat, std::optional<KHRMaterialClearcoat>& outClearcoat)
    {
        if (jClearcoat.contains("clearcoatFactor"))
            outClearcoat->ClearcoatFactor = (float) jClearcoat["clearcoatFactor"].get_number();

        if (jClearcoat.contains("clearcoatRoughnessFactor"))
            outClearcoat->ClearcoatRoughnessFactor = (float) jClearcoat["clearcoatRoughnessFactor"].get_number();

        if (jClearcoat.contains("clearcoatTexture"))
        {
            if (!ParseTextureInfo(jClearcoat.at("clearcoatTexture"), outClearcoat->ClearcoatTexture = TextureInfo()))
                return false;
        }

        if (jClearcoat.contains("clearcoatRoughnessTexture"))
        {
            if (!ParseTextureInfo(jClearcoat.at("clearcoatRoughnessTexture"), outClearcoat->ClearcoatRoughnessTexture = TextureInfo()))
                return false;
        }

        if (jClearcoat.contains("clearcoatNormalTexture"))
        {
            if (!ParseTextureInfo(jClearcoat.at("clearcoatNormalTexture"), outClearcoat->ClearcoatNormalTexture = TextureInfo()))
                return false;
        }

        return true;
    }

    bool JsonParser::ParseSheen(const glz::json_t& jSheen, std::optional<KHRMaterialSheen>& outSheen)
    {
        if (jSheen.contains("sheenColourTexture"))
        {
            if (!ParseTextureInfo(jSheen.at("sheenColourTexture"), outSheen->SheenColourTexture = TextureInfo()))
                return false;
        }

        if (jSheen.contains("sheenRoughnessTexture"))
        {
            if (!ParseTextureInfo(jSheen.at("sheenRoughnessTexture"), outSheen->SheenRoughnessTexture = TextureInfo()))
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

    bool JsonParser::ParseTransmission(const glz::json_t& jTransmission, std::optional<KHRMaterialTransmission>& outTransmission)
    {
        if (jTransmission.contains("transmissionFactor"))
            outTransmission->TransmissionFactor = (float) jTransmission.at("transmissionFactor").get_number();

        if (jTransmission.contains("transmissionTexture"))
        {
            if (!ParseTextureInfo(jTransmission.at("transmissionTexture"), outTransmission->TransmissionTexture = TextureInfo()))
                return false;
        }

        return true;
    }

    bool JsonParser::ParseVolume(const glz::json_t& jVolume, std::optional<KHRMaterialVolume>& outVolume)
    {
        if (jVolume.contains("thicknessFactor"))
            outVolume->ThicknessFactor = (float) jVolume.at("thicknessFactor").get_number();

        if (jVolume.contains("thicknessTexture"))
        {
            if (!ParseTextureInfo(jVolume.at("thicknessTexture"), outVolume->ThicknessTexture = TextureInfo()))
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

    void JsonParser::ParseIOR(const glz::json_t& jIOR, std::optional<KHRMaterialIOR>& outIOR)
    {
        if (jIOR.contains("ior")) {
            outIOR->IOR = (float) jIOR.at("ior").get_number();
        }
    }

    void JsonParser::ParseEmissiveStrength(const glz::json_t& jEmissiveStrength, std::optional<KHRMaterialEmissiveStrength>& outEmissiveStrength)
    {
        if (jEmissiveStrength.contains("emissiveStrength"))
            outEmissiveStrength->EmissiveStrength = (float) jEmissiveStrength.at("emissiveStrength").get_number();

    }

    bool JsonParser::ParseIridescence(const glz::json_t& jIridescence, std::optional<KHRMaterialIridescence>& outIridescence)
    {
        if (jIridescence.contains("iridescenceFactor"))
            outIridescence->IridescenceFactor = (float)jIridescence["iridescenceFactor"].get_number();

        if (jIridescence.contains("iridescenceTexture"))
        {
            if (!ParseTextureInfo(jIridescence.at("iridescenceTexture"), outIridescence->IridescenceTexture = TextureInfo()))
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
            if (!ParseTextureInfo(jIridescence.at("iridescenceThicknessTexture"), outIridescence->IridescenceThicknessTexture = TextureInfo()))
                return false;
        }

        return true;
    }

    bool JsonParser::ParseAnisotropy(const glz::json_t& jAnisotropy, std::optional<KHRMaterialAnisotropy>& outAnisotropy)
    {
        if (jAnisotropy.contains("anisotropyFactor"))
            outAnisotropy->AnisotropyFactor = (float)jAnisotropy["anisotropyFactor"].get_number();

        if (jAnisotropy.contains("anisotropyTexture"))
        {
            if (!ParseTextureInfo(jAnisotropy.at("anisotropyTexture"), outAnisotropy->AnisotropyTexture = TextureInfo()))
                return false;
        }

        if (jAnisotropy.contains("anisotropyDirection"))
        {
            auto& jAnisotropyDirection = jAnisotropy.at("anisotropyDirection").get_array();
            outAnisotropy->AnisotropyDirection = glm::vec3(
                    jAnisotropyDirection.at(0).get_number(),
                    jAnisotropyDirection.at(1).get_number(),
                    jAnisotropyDirection.at(2).get_number());
        }

        return true;
    }

    bool JsonParser::ParsePrimitives(const glz::json_t& jPrimitives, Mesh& mesh)
    {
        auto& primitives = jPrimitives.get_array();

        for (int i = 0; i < primitives.size(); i++)
        {
            const glz::json_t& jPrimitive = primitives.at(i);

            if (!jPrimitive.contains("attributes"))
            {
                OCASI_FAIL("Required 'primitives' property in mesh primitive is not present, though mandatory.");
                return false;
            }

            Primitive& primitive = mesh.Primitives.emplace_back(i);
            ParseVertexAttributes(jPrimitive.at("attributes"), primitive.Attributes);

            if (jPrimitive.contains("indices"))
                primitive.Indices = (size_t) jPrimitive.at("indices").get_number();

            if (jPrimitive.contains("material"))
                primitive.MaterialIndex = (size_t) jPrimitive.at("material").get_number();

            if (jPrimitive.contains("mode"))
                primitive.Type = (PrimitiveType) jPrimitive.at("mode").get_number();

            if (jPrimitive.contains("targets"))
            {
                auto& targets = jPrimitive.at("targets").get_array();

                for (int j = 0; j < targets.size(); j++)
                {
                    ParseVertexAttributes(targets.at(j), primitive.MorphTargets.at(j));
                }
            }
        }

        return true;
    }

    void JsonParser::ParseVertexAttributes(const glz::json_t& jVertexAttribute, VertexAttributes &outAttributes)
    {
        auto& attributes = jVertexAttribute.get_object();

        for (auto& [key, value] : attributes)
        {
            outAttributes[key] = (size_t) value.get_number();
        }
    }

    void JsonParser::ParseVec3(const glz::json_t& jVec, glm::vec3& out)
    {
        auto& vec = jVec.get_array();

        out = glm::vec3(
                vec.at(0).get_number(),
                vec.at(1).get_number(),
                vec.at(2).get_number());
    }

    void JsonParser::ParseVec4(const glz::json_t& jVec, glm::vec4& out)
    {
        auto& vec = jVec.get_array();

        out = glm::vec4(
                vec.at(0).get_number(),
                vec.at(1).get_number(),
                vec.at(2).get_number(),
                vec.at(3).get_number());
    }
}