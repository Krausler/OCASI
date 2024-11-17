#include "GLTFImporter.h"

namespace OCASI {

    GLTFImporter::GLTFImporter(FileReader &reader)
        : m_FileReader(reader)
    {}

    bool GLTFImporter::CanLoad()
    {
        return false;
    }

    std::shared_ptr<Asset> GLTFImporter::Load3DFile()
    {
        return std::shared_ptr<Asset>();
    }
}