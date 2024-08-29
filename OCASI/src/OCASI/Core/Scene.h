#pragma once

#include "OCASI/Core/Mesh.h"
#include "OCASI/Core/Material.h"

namespace OCASI {

    struct ObjectGroupingInfo
    {
        std::string Name;
        std::vector<uint32_t> MeshIndices;
    };

    struct Scene
    {
        std::vector<Mesh> Meshes;
        // A mesh's material is at the mesh's index in the mesh vector
        std::vector<Material> Materials;
        std::vector<ObjectGroupingInfo> ObjectGroupingInfo;
    };

}