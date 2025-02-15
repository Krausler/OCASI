OCASI (Octopus Asset Importer)
================================

OCASI is a multi-platform 3D asset importer, designed specifically for the [Octopus]('https://github.com/Krausler/Octopus' "Octopus engine link") engine.
It currently supports Windows and Linux with feature support for Linux in mind.

OCASI is licensed under the [MIT](LICENSE) license.

## Building OCASI

Currently the easiest way to include OCASI into your project is by adding it to a submodule. To do so, follow these steps:
1. Run `git submodule add https://github.com/Krausler/OCASI <desired folder>`.
2. Add `add_subdirectory(<path/to/OCASI>)` to your cmake file.
3. Add OCASI to your target executable / library using `target_link_libraries(<your target> PUBLIC/PRIVATE OCASI)`.

If you want to build OCASI from sources, consider that OCASI uses the libraries GLM, simdjson, spdlog and stbimage.

## Using OCASI

### Quick Start

Loading a model using OCASI is extremely simple:
```c++
#include "OCASI/Importer.h"

#include <iostream>

int main()
{
    using namespace OCASI;
    
    auto scene = Importer::Load3DFile();
    
    if (!scene)
        std::cout << "Failed to load model"
    else
    {
        ...
    }
}
```

When `OCASI::Importer::Load3DModel` returns a scene in form of a `SharedPtr<Scene>`, 
which is an alias for a `std::shared_ptr`. The scene struct contains models, meshes and a collection of root nodes.

The models vector contains models loaded from the 3D file and is a collection of meshes, associated with a name. 
If the only thing you want is to get the vertex data from the loaded meshes, you can do so by iterating over the models and their meshes:

```c++
for (auto& model : scene->Models)
{
    for (auto& mesh : model.Meshes)
    {
        auto vertices = mesh.Vertices;
        auto normals = mesh.Normals;
        ...
    }
}
```

A mesh contains its vertex positions, possibly vertex colours, normals, multiple sets of texture coordinates, tangents,
and indices. To check whether they are present it is recommended to check if these arrays are empty. 
Additionally, each mesh has a `MaterialIndex` project that is either a valid index into the scenes material array or `INVALID_ID`.

### Materials

Materials describe how a mesh should be rendered. An OCASI material can be read as following:

```c++
auto& material = scene->Materials.at(materialIndex); 

float roughness = material.GetValue<float>(MATERIAL_ALBEDO_COLOUR);

if (material.HasTexture(MATERIAL_TEXTURE_NORMAL))
{
    auto normalTexture = material.GetTexture(MATERIAL_TEXTURE_NORMAL);
    ...
}
```

All available material values and texture indices can be found in the [Model.h](OCASI/src/OCASI/Core/Model.h) header.

### Images

OCASI uses Images as its type of textures. Images can be memory only or contain a valid path to an image that is the texture. 
Loading the actual texture data can be either, handled by the user or using the inbuilt functions which use `stbimage`.
When the image is memory only, the compressed image data is stored in the `ImageData`'s `Data` field, when the image
is associated with a path the image data will be empty. However, when you load it using the inbuilt functionality `ImageData`
will contain the correct number of channels, width and height. When loading fails, nullptr is returned.

```c++
auto normalTexture = material.GetTexture(MATERIAL_TEXTURE_NORMAL);

// Handle image loading yourself
{
    if (!normalTexture->IsMemoryImage())
    {
        LoadFromPath(normalTexture->GetImagePath());
    }
    else
    {
        LoadFromMemory(normalTexture->GetImageData().Data);
    }
}

{
    auto imageData = normalTexture->Load();
    
    if (!imageData)
        std::cout << "Failed to load model"
}
```

### Nodes

If you want to parse complex scenes with node-hierarchy-structures the `RootNodes` property of a scene will be your friend.
It contains the root nodes of the scene hierarchy. Nodes contain a local matrix, a model index, 
that is either a valid index into the scenes `Models` array or INVALID_ID, the parent node and it's children. Nodes with no models
are structural indicators of the scene. To get the global world model matrix of a model all matrices have to be multiplied against each other. 