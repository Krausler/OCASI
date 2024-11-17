#pragma once

#include "OCASI/Core/BaseImporter.h"
#include "OCASI/Core/FileUtil.h"

namespace OCASI {

    class GLTFImporter : public BaseImporter
    {
    public:
        GLTFImporter(FileReader& reader);
        ~GLTFImporter() = default;

        virtual bool CanLoad() override;
        virtual std::shared_ptr<Asset> Load3DFile() override;
    private:
        FileReader& m_FileReader;
    };

}
