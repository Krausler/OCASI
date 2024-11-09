#include "OCASI/Core/BaseImporter.h"

#include "OCASI/Importers/OBJ/Model.h"
#include "OCASI/Core/FileUtil.h"

namespace OCASI {
    struct VertexIndices;

    class ObjImporter : public BaseImporter{
    public:
        ObjImporter(FileReader& reader);
        ~ObjImporter() = default;

        virtual std::shared_ptr<Scene> Load3DFile() override;
        virtual bool CanLoad() override;
    private:
        std::shared_ptr<Scene> ConvertToOCASIScene(const std::shared_ptr<OBJ::Model>& model, const Path& folder) const;
        std::shared_ptr<Node> CreateNodesForObject(const OBJ::Object& o) const;

        void CreateNewVertex(Mesh& mesh, const std::shared_ptr<OBJ::Model>& model, const VertexIndices& indices) const;
    private:
        FileReader& m_FileReader;
    };

}