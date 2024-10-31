#include "ObjImporter.h"

#include "OCASI/Importers/OBJ/ObjFileParser.h"

#include <fstream>

namespace OCASI {

    ObjImporter::ObjImporter(FileReader& reader)
        : m_FileReader(reader)
    {
    }

    std::shared_ptr<Scene> ObjImporter::Load3DFile() {
        OBJ::ObjFileParser fileParser(m_FileReader);
        auto model = fileParser.ParseOBJFile();

        if (!model)
            return nullptr;

        return ConvertToOCASIScene(model);
    }

    bool ObjImporter::CanLoad() {
        return Util::FindTokensInFirst100Lines(m_FileReader, { "v", "vn", "vt", "mtlib", "f", "usemtl" });
    }

    std::shared_ptr<Scene> ObjImporter::ConvertToOCASIScene(const std::shared_ptr<OBJ::Model> &model) const
    {
        std::shared_ptr<Scene> scene = std::make_shared<Scene>();

        for (const OBJ::Mesh& m : model->Meshes)
        {
            Mesh& newMesh = scene->Meshes.emplace_back();
            newMesh.Name = m.Name;
            newMesh.Vertices = m.Vertices;
            // OBJ files only supports 1 set of texture coordinates
            newMesh.TexCoords[0] = m.TexCoords;
            newMesh.Normals = m.Normals;
            newMesh.Indices = m.Indices;
            newMesh.FaceType = m.FaceType;
            newMesh.Dimension = m.Dimension;
            newMesh.HasNormals = !newMesh.Normals.empty();
            newMesh.HasTexCoords = !newMesh.TexCoords.empty();
            // OBJ files do not support tangents
            newMesh.HasTangents = false;
        }

        for (const OBJ::Object& o : model->Objects)
        {
            scene->RootNodes.push_back(CreateNodesForObject(o));
        }

        return scene;
    }

    std::shared_ptr<Node> ObjImporter::CreateNodesForObject(const OBJ::Object& o) const
    {
        std::shared_ptr<Node> root = std::make_shared<Node>();
        root->Parent = nullptr;
        root->MeshIndex = o.ParentMesh;

        for(uint32_t m : o.Children)
        {
            std::shared_ptr<Node>& child = root->Children.emplace_back();
            child->Parent = root;
            child->MeshIndex = m;
        }
        // TODO: MaterialIndex
        return root;
    }
}
