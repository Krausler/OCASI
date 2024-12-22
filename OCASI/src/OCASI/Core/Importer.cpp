#include "Importer.h"

#include "OCASI/Importers/OBJ/ObjImporter.h"
#include "OCASI/Importers/GLTF2//GLTFImporter.h"

#include <unordered_map>

#define CAN_LOAD(x, fName) if(!x.CanLoad()) { OCASI_LOG_ERROR("Can't load file as CanLoad() for {} did not succeed.", fName); return nullptr; }

namespace OCASI {

    void Importer::Init()
    {
        Logger::Init();
    }

    std::shared_ptr<Scene> Importer::Load3DFile(const Path &path)
    {
        if(!exists(path))
        {
            OCASI_FAIL("Requested file does not exist. Verify the 3D model path.");
            return nullptr;
        }

        std::string fExtension = path.extension().string();
        std::shared_ptr<Scene> result = nullptr;

        // TODO: Change this and make it make sense that all importers share a base class
        if(fExtension == ".obj")
        {
            Logger::SetFileFormatPattern("OBJ");

            FileReader reader(path, false);

            ObjImporter importer(reader);
            CAN_LOAD(importer, path.string());
            result = importer.Load3DFile();
        }
        Logger::ResetPattern();


        if (fExtension == ".gltf" || fExtension == ".glb")
        {
            Logger::SetFileFormatPattern("GLTF");

            bool binary = fExtension == ".glb";
            FileReader reader(path, binary);

            GLTFImporter importer(reader);
            CAN_LOAD(importer, path.string());
            result = importer.Load3DFile();
        }
        Logger::ResetPattern();

        OCASI_ASSERT(result);

        return result;
    }
}
