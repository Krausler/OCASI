#pragma once

#include "OCASI/Core/Model.h"
#include "OCASI/Core/Material.h"

namespace OCASI {

    /*! @brief A geometry node holding a parent and a collection of children, along with an optional model index and a transform of that
     *         node/model.
     *
     *  Nodes can have the state of being empty when they do not have a valid model index. In this case they serve as some kind of
     *  'holder node', being the owner of subsequent children nodes. The absolute position of that model is specified using the
     *  LocalTransform variable.
     *
     *  The difference between a models submeshes and the node hierarchy is that the nodes models, in a render implementation, can be moved
     *  individually, regardless of how deep the hierarchy goes or how many children a node has. A models submeshes on the other hand, are
     *  a separation of a single meshes vertex data, with these separate meshes having different materials.
     */
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
        
        bool IsEmpty() const { return ModelIndex == INVALID_ID; }

        SharedPtr<Node> Parent;
        std::vector<SharedPtr<Node>> Children;

        size_t ModelIndex = INVALID_ID;
        glm::mat4 LocalTransform = glm::mat4(1.0f);
    };

    /*! @brief The scene is the output product of loading and afterwards post processing a 3D model file.
     *
     *  To start parsing this structure, recursively read in the RootNodes, until you reach a leave node, that has no more children
     *  to be parsed. When you reach a node that is not empty, use the ModelIndex in that node as an index into the Models array.
     *  To get a material of a mesh do the same with the MaterialIndex, found inside the Mesh struct.
     */
    struct Scene
    {
        std::vector<Model> Models;
        std::vector<Material> Materials;
        std::vector<SharedPtr<Node>> RootNodes;
    };

}