#include "OCASI/Core/Importer.h"
#include "OCASI/Core/StringUtil.h"

#include <iostream>

int main()
{
    auto sceneError = OCASI::Importer::Load3DFile("Resources/GLTF/Mushroom.gltf", OCASI::PostProcessorOptions::None);
    if (!sceneError)
    {
        std::cerr << sceneError.error().GetMessage() << std::endl;
        return 1;
    }
    
    auto scene = sceneError.value();
    
    if (scene->HasMaterials())
    {
        auto mat = scene->Materials.at(0);
        
        float roughness = mat.GetValue<float>(OCASI::MATERIAL_ROUGHNESS);
        OCASI_LOG_INFO("{}", roughness);
        
        auto texture = mat.GetTexture(OCASI::MATERIAL_TEXTURE_NORMAL);
        OCASI_ASSERT(texture);
        
        OCASI_LOG_INFO("{}", texture->IsLoaded());
        const auto& data = texture->Load();
        OCASI_LOG_INFO("{}", texture->IsLoaded());
        OCASI_LOG_INFO("{}", data->Width);
    }
}
