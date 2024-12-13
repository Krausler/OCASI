add_library(OCASI STATIC
        "src/OCASI/Core/Base.h"
        "src/OCASI/Core/Debug.h"
        "src/OCASI/Core/Importer.h"
        "src/OCASI/Core/Importer.cpp"
        "src/OCASI/Core/BaseImporter.h"
        "src/OCASI/Core/Logger.cpp"
        "src/OCASI/Core/Logger.h"
        "src/OCASI/Core/Model.h"
        "src/OCASI/Core/Material.h"
        "src/OCASI/Core/Scene.h"
        "src/OCASI/Core/Image.h"
        "src/OCASI/Core/Image.cpp"
        "src/OCASI/Importers/OBJ/ObjImporter.cpp"
        "src/OCASI/Importers/OBJ/ObjImporter.h"
        src/OCASI/Importers/OBJ/Model.h
        src/OCASI/Core/FileUtil.cpp
        src/OCASI/Core/FileUtil.h
        src/OCASI/Importers/OBJ/FileParser.cpp
        src/OCASI/Importers/OBJ/FileParser.h
        src/OCASI/Core/StringUtil.cpp
        src/OCASI/Core/StringUtil.h
        src/OCASI/Importers/OBJ/MtlParser.cpp
        src/OCASI/Importers/OBJ/MtlParser.h
        src/OCASI/Importers/GLTF2/GLTFImporter.cpp
        src/OCASI/Importers/GLTF2/GLTFImporter.h
        src/OCASI/Importers/GLTF2/Asset.h
        src/OCASI/Importers/GLTF2/Asset.cpp
        src/OCASI/Importers/GLTF2/JsonParser.cpp
        src/OCASI/Importers/GLTF2/JsonParser.h
        src/OCASI/Core/BinaryReader.h
        src/OCASI/Core/BinaryReader.cpp
        src/OCASI/Core/Material.cpp
)

target_include_directories(OCASI PUBLIC
        src
        vendor/spdlog/include
        vendor/stbimage
        vendor/glm
        vendor/glaze/include
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(OCASI PUBLIC
            "OCASI_DEBUG"
        )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_definitions(OCASI PUBLIC
            "OCASI_RELEASE"
        )
endif()