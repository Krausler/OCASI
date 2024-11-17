#pragma once

#include "OCASI/Core/Scene.h"

namespace OCASI {

    class BaseImporter 
    {
    public:
        virtual std::shared_ptr<Asset> Load3DFile() = 0;
        virtual bool CanLoad() = 0;
    };
}