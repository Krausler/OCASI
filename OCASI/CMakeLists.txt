add_library(OCASI STATIC
    "src/OCASI/Core/Base.h"
    "src/OCASI/Core/Debug.h"
    "src/OCASI/Core/Importer.h"
    "src/OCASI/Core/Importer.cpp"
    "src/OCASI/Core/BaseImporter.h"
    "src/OCASI/Core/Logger.cpp"
    "src/OCASI/Core/Logger.h"
    "src/OCASI/Core/Mesh.h"
        "src/OCASI/Core/Material.h"
    "src/OCASI/Core/Scene.h"
    "src/OCASI/Core/Image.h"
    "src/OCASI/Core/Image.cpp"
    "src/OCASI/Importers/OBJ/ObjImporter.cpp"
    "src/OCASI/Importers/OBJ/ObjImporter.h"
        src/OCASI/Importers/OBJ/OBJModel.h
        src/OCASI/Core/FileUtil.cpp
        src/OCASI/Core/FileUtil.h
        src/OCASI/Importers/OBJ/ObjFileParser.cpp
        src/OCASI/Importers/OBJ/ObjFileParser.h
        src/OCASI/Core/StringUtil.cpp
        src/OCASI/Core/StringUtil.h
)

target_include_directories(OCASI PUBLIC
    src
    vendor
    vendor/spdlog/include
    vendor/glm
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