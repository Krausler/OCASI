#include "OCASI/Core/BaseImporter.h"

#include "OCASI/Importers/OBJ/Model.h"
#include "OCASI/Core/FileUtil.h"

namespace OCASI {
    struct VertexIndices;

    class ObjImporter : public BaseImporter
    {
    public:
        virtual bool CanLoad(FileReader& reader) override;
        virtual std::shared_ptr<Scene> Load3DFile(FileReader& reader) override;
        
        virtual std::string_view GetLoggerPattern() const override { return "OBJ"; }
        virtual const std::vector<std::string_view> GetSupportedFileExtensions() const override { return { ".obj" }; }
        virtual ImporterType GetImporterType() const override { return ImporterType::OBJ; }
    private:
        std::shared_ptr<Scene> ConvertToOCASIScene(const Path& folder);
        std::shared_ptr<Node> CreateNodes(const OBJ::Object& o);

        Mesh CreateMesh(size_t mesh) const;
        void CreateNewVertex(Mesh& mesh, const VertexIndices& indices) const;
        void SortTextures(Material& newMat, const OBJ::Material& mat, const Path& folder, size_t i);
    private:
        FileReader* m_FileReader = nullptr;

        std::shared_ptr<OBJ::Model> m_OBJModel = nullptr;
        std::shared_ptr<Scene> m_OutputScene = nullptr;
    };

}