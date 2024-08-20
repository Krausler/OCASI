#pragma once

#include "OCASI/Core/Base.h"
#include "OCASI/Core/Scene.h"

namespace OCASI {

    class BaseImporter 
    {
    public:
        virtual std::shared_ptr<Scene> Load3DFile(const Path& path) = 0;
    }

}