#include "OCASI/Core/Importer.h"
#include "OCASI/Core/StringUtil.h"

int main()
{
    auto scene = OCASI::Importer::Load3DFile("Resources/OBJ/TestObject.obj", OCASI::PostProcessorOptions::None);

    OCASI::Material& mat = scene->Materials.at(0);

    float roughness = mat.GetValue<float>(OCASI::MATERIAL_ROUGHNESS);
    OCASI_LOG_INFO("{}", roughness);

    auto texture = mat.GetTexture(OCASI::MATERIAL_TEXTURE_NORMAL);
    OCASI_ASSERT(texture);

    OCASI_LOG_INFO(texture->IsLoaded());
    const auto& data = texture->Load();
    OCASI_LOG_INFO(texture->IsLoaded());
    OCASI_LOG_INFO(data.Width);
}