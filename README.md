OCASI (Octopus Scene Importer)
================================

OCASI is (going to be) an easy simple asset importer specifically designed for the octopus engine. Planned support for formats are Wavefront OBJ, GLTF 2.0 and FBX

## Getting Started
To build the library start by cloning the repository using `git clone htpps://github.com/Krausler/OCASI`. OCASI uses CMake as a build system:
1. Create a new directory in the projects root folder called build or similar. (`mkdir build`)
2. Open a terminal in that folder and run `cmake ..` to create build files for your platform or run `cmake .. --build` to build the project.

Alternativley include the OCASI library in your cmake project to build it along side with your project.

## Library Usage
OCASI is build to be as simple as possible for users. To load an asset file of the supported formats, first initialize the library via `OCASI::Importer::Init()`, then run `OCASI::Importer::Load3DModel("path/to/file")`.
