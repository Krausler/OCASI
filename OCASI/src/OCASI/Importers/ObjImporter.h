#include "OCASI/Core/BaseImporter.h"

namespace OCASI {

    class ObjImporter : public BaseImporter
    {
    public:
        virtual std::shared_ptr<Scene> Load3DFile(const Path& path) override;

    };

}