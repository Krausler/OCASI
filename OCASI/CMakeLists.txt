add_library(OCASI ${OCASI_LIBRARY_TYPE}
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
        "src/OCASI/Importers/OBJ/Model.h"
        "src/OCASI/Core/FileUtil.cpp"
        "src/OCASI/Core/FileUtil.h"
        "src/OCASI/Importers/OBJ/FileParser.cpp"
        "src/OCASI/Importers/OBJ/FileParser.h"
        "src/OCASI/Core/StringUtil.cpp"
        "src/OCASI/Core/StringUtil.h"
        "src/OCASI/Importers/OBJ/MtlParser.cpp"
        "src/OCASI/Importers/OBJ/MtlParser.h"
        "src/OCASI/Importers/GLTF2/GLTFImporter.cpp"
        "src/OCASI/Importers/GLTF2/GLTFImporter.h"
        "src/OCASI/Importers/GLTF2/Asset.h"
        "src/OCASI/Importers/GLTF2/Asset.cpp"
        "src/OCASI/Importers/GLTF2/JsonParser.cpp"
        "src/OCASI/Importers/GLTF2/JsonParser.h"
        "src/OCASI/Core/BinaryReader.h"
        "src/OCASI/Core/BinaryReader.cpp"
        "src/OCASI/Core/Material.cpp"
        "src/OCASI/Importers/GLTF2/Json.h"
        "src/OCASI/Core/PostProcessor.cpp"
        "src/OCASI/Core/PostProcessor.h"
        "src/OCASI/Core/PostProcessorOptions.h"
        "src/OCASI/Core/BasePostProcess.cpp"
        "src/OCASI/Core/BasePostProcess.h"
        "src/OCASI/PostProcessing/TriangulateProcess.cpp"
        "src/OCASI/PostProcessing/TriangulateProcess.h"
        "src/OCASI/Core/Error.h"
        "src/OCASI/PostProcessing/ConverToRHCProcess.cpp"
        "src/OCASI/PostProcessing/ConverToRHCProcess.h"
        src/OCASI/PostProcessing/GenerateNormalsProcess.cpp
        src/OCASI/PostProcessing/GenerateNormalsProcess.h
)

target_sources(OCASI PRIVATE vendor/simdjson/simdjson.cpp)

target_include_directories(OCASI PUBLIC
        src
        vendor/glm
)

target_include_directories(OCASI PRIVATE
        vendor/stbimage
        vendor/glm
        vendor/simdjson
)

target_compile_definitions(OCASI PUBLIC ${OCASI_PLATFORM_MACRO} SIMDJSON_EXCEPTIONS=OFF)

if (OCASI_CONFIGURATION STREQUAL "Debug")
        target_compile_definitions(OCASI PUBLIC "OCASI_DEBUG")
elseif(OCASI_CONFIGURATION STREQUAL "Release")
        target_compile_definitions(OCASI PUBLIC "OCASI_RELEASE")
endif()