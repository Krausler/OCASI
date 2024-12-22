#include "OCASI/Core/BaseImporter.h"

#include "OCASI/Importers/OBJ/Model.h"
#include "OCASI/Core/FileUtil.h"

namespace OCASI {
    struct VertexIndices;

    class ObjImporter : public BaseImporter
    {
    public:
        ObjImporter(FileReader& reader);
        ~ObjImporter() = default;

        virtual std::shared_ptr<Scene> Load3DFile() override;
        virtual bool CanLoad() override;
    private:
        std::shared_ptr<Scene> ConvertToOCASIScene(const Path& folder);
        std::shared_ptr<Node> CreateNodes(const OBJ::Object& o);

        Mesh CreateMesh(size_t mesh) const;
        void CreateNewVertex(Mesh& mesh, const VertexIndices& indices) const;
        void SortTextures(Material& newMat, const OBJ::Material& mat, const Path& folder, size_t i);
    private:
        FileReader& m_FileReader;

        std::shared_ptr<OBJ::Model> m_OBJModel = nullptr;
        std::shared_ptr<Scene> m_OutputScene = nullptr;
    };

}