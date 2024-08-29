#include "OCASI/Core/Base.h"

namespace OCASI::Util {

    std::vector<std::string> Split(const std::string& target, char token);
    std::vector<std::string> Split(const std::string& target, char token, uint32_t& outTokenCount);

}