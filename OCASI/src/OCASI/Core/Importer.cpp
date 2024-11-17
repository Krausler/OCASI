#include "Importer.h"

#include "OCASI/Importers/OBJ/ObjImporter.h"

#include <unordered_map>

#define CAN_LOAD(x, fName) if(!x.CanLoad()) { OCASI_LOG_ERROR("Can't load file as CanLoad() for {} did not succeed.", fName); return nullptr; }

namespace OCASI {

    void Importer::Init()
    {
        Logger::Init();
    }

    std::shared_ptr<Asset> Importer::Load3DFile(const Path &path)
    {
        FileReader reader(path);
        if(!reader)
        {
            OCASI_LOG_ERROR("Requested file does not exist. Verify the 3D model path.");
            OCASI_FAIL("Requested file does not exist. Verify the 3D model path.");
            return nullptr;
        }

        std::string fExtension = reader.GetPath().extension().string();
        std::shared_ptr<Asset> result = nullptr;
        if(fExtension == ".obj")
        {
            Logger::SetFileFormatPattern("OBJ");
            ObjImporter importer(reader);
            CAN_LOAD(importer, path.string());
            result = importer.Load3DFile();
        }
        Logger::ResetPattern();
        OCASI_ASSERT(result);

        return nullptr;
    }
}
