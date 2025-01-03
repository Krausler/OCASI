#include "OCASI/Core/Importer.h"
#include "OCASI/Core/StringUtil.h"

#include "glaze/glaze.hpp"

int main()
{
    OCASI::Importer::Init();

    OCASI_LOG_ERROR("hi");
    std::filesystem::current_path("C:/Lauri/Dev/C++/Projekte/Octopus/Projekte/OCASI/OCASI-Tester");

    auto scene = OCASI::Importer::Load3DFile("Resources/GLTF/Mushroom.glb");

    OCASI::Material& mat = scene->Materials.at(0);

    float roughness = mat.GetValue<float>(OCASI::MATERIAL_ROUGHNESS);
    OCASI_LOG_INFO("{}", roughness);

    auto texture = mat.GetTexture(OCASI::MATERIAL_TEXTURE_ALBEDO);
    OCASI_ASSERT(texture);

    OCASI_LOG_INFO(texture->IsLoaded());
    const auto& data = texture->Load();
    OCASI_LOG_INFO(texture->IsLoaded());
    OCASI_LOG_INFO(data.Width);
}