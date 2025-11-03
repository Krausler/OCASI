cmake_minimum_required(VERSION 3.6)

set(OCASI_VERSION 1.1.1)
project("Octopus Asset Importer" VERSION ${OCASI_VERSION})

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(OCASI_BUILD_TESTS "Build the OCASI-Tests project" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

if (MSVC)
  add_compile_options(/utf-8 /Zc:preprocessor)
endif ()

if (NOT TARGET OCBase)
  add_subdirectory(OCASI/vendor/OCBase)
endif ()
SetCMakeVariables(OCASI)

add_subdirectory(OCASI)
if (OCASI_BUILD_TESTS)
  add_subdirectory(Tests)
endif ()