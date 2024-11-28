#pragma once

#include "OCASI/Core/Mesh.h"
#include "OCASI/Core/Material.h"

namespace OCASI {

    const size_t INVALID_ID = -1;

    struct Node
    {
        Node()
        {
        }

        Node(const Node& other)
        {
            Parent = other.Parent;
            Children = other.Children;
            MeshIndex = other.MeshIndex;
            MaterialIndex = other.MeshIndex;
        }

        std::shared_ptr<Node> Parent;
        std::vector<std::shared_ptr<Node>> Children;

        size_t MeshIndex = INVALID_ID;
        size_t MaterialIndex = INVALID_ID;
    };

    struct Scene
    {
        std::vector<Mesh> Meshes;
        std::vector<Material> Materials;
        std::vector<std::shared_ptr<Node>> RootNodes;
    };

}