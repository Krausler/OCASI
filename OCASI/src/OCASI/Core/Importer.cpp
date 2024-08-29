#include "Importer.h"

#include "OCASI/Core/BaseImporter.h"
#include "OCASI/Importers/ObjImporter.h"

#include <unordered_map>

namespace OCASI {

    std::unordered_map<std::string, std::shared_ptr<BaseImporter>> FileExtensionToImporter;

    void Importer::Init()
    {
        Logger::Init();
        FileExtensionToImporter[".obj"] = std::make_shared<ObjImporter>();

    }

    std::shared_ptr<Scene> Importer::Load3DFile(const Path &path)
    {
        if(FileExtensionToImporter.find(path.extension().string()) == FileExtensionToImporter.end())
        {
            OCASI_LOG_ERROR("Supplied file with unsopported file extension: {}", path.extension().string());
            return nullptr;
        }

        return FileExtensionToImporter[path.extension().string()]->Load3DFile(path);
    }
}
