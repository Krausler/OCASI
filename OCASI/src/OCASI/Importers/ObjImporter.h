#include "OCASI/Core/BaseImporter.h"

namespace OCASI {

    class ObjImporter : public BaseImporter
    {
    public:
        virtual std::shared_ptr<Scene> Load3DFile(const Path& path) override;
    private:
        std::string ReadName(const std::string& line);

        glm::vec3 ParseVec3(const std::string& line);
        glm::vec2 ParseVec2(const std::string& line);
    };

}