#include "OCASI/Core/BaseImporter.h"

#include "OCASI/Importers/OBJ/OBJModel.h"
#include "OCASI/Core/FileUtil.h"

namespace OCASI {
    class ObjImporter : public BaseImporter{
    public:
        ObjImporter(FileReader& reader);
        ~ObjImporter() = default;

        virtual std::shared_ptr<Scene> Load3DFile() override;
        virtual bool CanLoad() override;
    private:
        std::shared_ptr<Scene> ConvertToOCASIScene(const std::shared_ptr<OBJ::Model>& model) const;
        std::shared_ptr<Node> CreateNodesForObject(const OBJ::Object& o) const;
    private:
        FileReader& m_FileReader;
    };

}