#pragma once

#include "OCasi/Core/Scene.h"

namespace OCASI {
    class Importer 
    {
    public:
        static void Init();

        static std::shared_ptr<Asset> Load3DFile(const Path& path);
    };
}