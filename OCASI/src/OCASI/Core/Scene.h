#pragma once

#include "OCASI/Core/Model.h"
#include "OCASI/Core/Material.h"

namespace OCASI {

    struct Node
    {
        Node()
        {
        }

        Node(const Node& other)
        {
            Parent = other.Parent;
            Children = other.Children;
            ModelIndex = other.ModelIndex;
            LocalTransform = other.LocalTransform;
        }

        SharedPtr<Node> Parent;
        std::vector<SharedPtr<Node>> Children;

        size_t ModelIndex = INVALID_ID;
        glm::mat4 LocalTransform = glm::mat4(1.0f);
    };

    struct Scene
    {
        std::vector<Model> Models;
        std::vector<Material> Materials;
        std::vector<SharedPtr<Node>> RootNodes;
    };

}