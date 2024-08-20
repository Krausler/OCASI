#include "Importer.h"

#include "OCASI/Core/BaseImporter.h"

#include <unorderd_map>

namespace OCASI {

    std::unordered_map<std::string, std::shared_ptr<BaseImporter>> FileExtensionToImporter;


    void Importer::Init()
    {
        Logger::Init();
        FileExtensionToImporter[".obj"] = std::make_shared<ObjImporter>();

    }

    std::shared<Scene> Importer::Load3DFile(const Path &path)
    {
        if(FileExtensionImporter.at(path.extension()) == nullptr)
        {
            OCASI_LOG_ERROR("Supplied file with unsopported file extension: {}", path.extension());
            return nullptr;
        }

        return FileExtensionImporter[path.extension()].Load3DFile(path);
    }
}
