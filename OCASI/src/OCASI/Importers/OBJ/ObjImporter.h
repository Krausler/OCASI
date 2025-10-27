#include "OCASI/Core/BaseImporter.h"

#include "OCASI/Importers/OBJ/Model.h"

namespace OCASI {
    struct VertexIndices;

    class ObjImporter : public BaseImporter
    {
    public:
        virtual bool CanLoad(OCBase::FileStreamReader& reader) override;
        virtual ExpectedImportT<SharedPtr<Scene>> Load3DFile(OCBase::FileStreamReader& reader) override;
        
        virtual const Vector<std::string_view> GetSupportedFileExtensions() const override { return { ".obj" }; }
        virtual ImporterType GetImporterType() const override { return ImporterType::OBJ; }
    private:
        SharedPtr<Scene> ConvertToOCASIScene(const Path& folder);
        SharedPtr<Node> CreateNodes(const OBJ::Object& o);

        Mesh CreateMesh(size_t mesh) const;
        void CreateNewVertex(Mesh& mesh, const VertexIndices& indices, size_t newIndex) const;
        ExpectedImport SortTextures(Material& newMat, const OBJ::Material& mat, const Path& folder, size_t i);
    private:
        SharedPtr<OBJ::Model> m_OBJModel = nullptr;
        SharedPtr<Scene> m_OutputScene = nullptr;
    };

}