OCASI (Octopus Asset Importer)
================================

OCASI is a 3D asset importer, designed specifically for the [Octopus]('https://github.com/Krausler/Octopus' "Octopus engine link") engine.

## Getting Started
To build the library start by cloning the repository using `git clone htpps://github.com/Krausler/OCASI`. OCASI uses CMake as a build system:
1. Create a new directory in the projects root folder called build or similar. (`mkdir build`)
2. Open a terminal in that folder and run `cmake ..` to create build files for your platform or run `cmake .. --build` to build the project.

Alternativley include the OCASI library in your cmake project to build it alongside with your project.

## Library Usage
OCASI is build to be as simple as possible for users. To load an asset file of the supported formats, 
initialize the library via `OCASI::Importer::Init()`, then run `OCASI::Importer::Load3DModel("path/to/file")`.
