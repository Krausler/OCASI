#pragma once

#include "OCASI/Core/Model.h"
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
        }

        std::shared_ptr<Node> Parent;
        std::vector<std::shared_ptr<Node>> Children;

        std::string Name;
        size_t MeshIndex = INVALID_ID;
        glm::mat4 LocalTransform = glm::mat4(1.0f);
    };

    struct Scene
    {
        std::vector<Model> Models;
        std::vector<Material> Materials;
        std::vector<std::shared_ptr<Node>> RootNodes;
    };

}