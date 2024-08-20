#pragma once

#include "OCASI/Core/Base.h"
#include "OCasi/Core/Scene.h"

namespace OCASI {
    class Importer 
    {
    public:
        static void Init();
        
        static std::shared<Scene> Load3DFile(const Path& path);
    };
}