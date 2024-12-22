#include "OCASI/Core/Importer.h"
#include "OCASI/Core/StringUtil.h"

#include "glaze/glaze.hpp"

int main()
{
    OCASI::Importer::Init();

    OCASI_LOG_ERROR("hi");
    std::filesystem::current_path("C:/Lauri/Dev/C++/Projekte/Octopus/Projekte/OCASI/Tester");

    auto scene = OCASI::Importer::Load3DFile("Resources/GLTF/Mushroom.gltf");
}