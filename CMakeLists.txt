cmake_minimum_required(VERSION 3.16)

set(OCTOPUS_ASSET_IMPORTER_VERSION 0.0.1)

project("Octopus Asset Importer" VERSION ${OCTOPUS_ASSET_IMPORTER_VERSION})

set(OCTOPUS_ASSET_IMPORTER_BUILD_TYPES "Debug" "Release" "Profile")
get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(multi_config)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "list of supported configuration types" FORCE)
elseif(NOT CMAKE_BUILD_TYPE STREQUAL "")
  if(NOT CMAKE_BUILD_TYPE IN_LIST OCTOPUS_ASSET_IMPORTER_BUILD_TYPES)
    message(FATAL_ERROR
      "Invalid build type '${CMAKE_BUILD_TYPE}'.
      CMAKE_BUILD_TYPE must be any one of the possible values:
      ${OCTOPUS_BUILD_TYPES}"
    )
  endif()
else()
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build Type of the project.")
endif()

message("Octopus Asset Importer: Build Configuration: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 20)


if(MSVC)
  add_compile_options("/utf-8")
endif()

add_subdirectory(OCASI)
add_subdirectory(Tester)