cmake_minimum_required(VERSION 3.6)

set(OCASI_VERSION 0.1.1)
project("Octopus Asset Importer" VERSION ${OCASI_VERSION})

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(OCASI_BUILD_TESTS "Build the OCASI-Tests project" ON)

#------------------------------------------------------------------------------
# Build configuration and platform
#------------------------------------------------------------------------------

set(OCASI_CONFIGURATION ${CMAKE_BUILD_TYPE})
set(OCASI_AVAILABLE_CONFIGURATIONS "Debug" "Release")

if (OCASI_CONFIGURATION IN_LIST OCASI_AVAILABLE_CONFIGURATIONS)
  message(STATUS "OCASI: Building OCASI using ${OCASI_CONFIGURATION} configuration")
else ()
  message(FATAL_ERROR "OCASI: Unsupported configuration. Please use one of these: ${OCASI_AVAILABLE_CONFIGURATIONS}")
endif ()

if (WIN32)
  message(STATUS "OCASI: Detected platform WIN32")
  set(OCASI_PLATFORM_MACRO "OCASI_PLATFORM_WINDOWS")
elseif (UNIX AND NOT APPLE)
  message(STATUS "OCASI: Detected platform UNIX")
  set(OCASI_PLATFORM_MACRO "OCASI_PLATFORM_LINUX")
else ()
  message(FATAL_ERROR "OCASI: Unsupported platform")
endif ()

#------------------------------------------------------------------------------
# Library type
#------------------------------------------------------------------------------

if (BUILD_SHARED_LIBS)
  set(OCASI_LIBRARY_TYPE "SHARED")
else ()
  set(OCASI_LIBRARY_TYPE "STATIC")
endif ()

message(STATUS "OCASI: Building OCASI with library type ${OCASI_LIBRARY_TYPE}")

#------------------------------------------------------------------------------
# Global compile/link settings
#------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

if (MSVC)
  add_compile_options(/utf-8 /Zc:preprocessor)
endif ()


add_subdirectory(OCASI)

if (OCASI_BUILD_TESTS)
  add_subdirectory(Tests)
endif ()