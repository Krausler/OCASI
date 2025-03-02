#include "OCASI/Core/Importer.h"
#include "OCASI/Core/StringUtil.h"

int main()
{
    OCASI_LOG_INFO(FORMAT("File Path: {}", std::filesystem::current_path().string()));

    auto scene = OCASI::Importer::Load3DFile("Resources/GLTF/Mushroom.glb", OCASI::PostProcessorOptions::None);

    OCASI::Material& mat = scene->Materials.at(0);

    float roughness = mat.GetValue<float>(OCASI::MATERIAL_ROUGHNESS);
    OCASI_LOG_INFO(FORMAT("{}", roughness));

    auto texture = mat.GetTexture(OCASI::MATERIAL_TEXTURE_NORMAL);
    OCASI_ASSERT(texture);

    OCASI_LOG_INFO(FORMAT("{}", texture->IsLoaded()));
    const auto& data = texture->Load();
    OCASI_LOG_INFO(FORMAT("{}", texture->IsLoaded()));
    OCASI_LOG_INFO(FORMAT("{}", data->Width));
}
