#pragma once

#include "OCASI/Core/Base.h"
#include "OCASI/Core/Mesh.h"
#include "OCASI/Core/Material.h"

namespace OCASI {

    struct Scene
    {
        std::vector<Mesh> Meshes;
        // A mesh's Material is at the mesh's index in the mesh vector
        std::vector<Material> Materials;
    };

}