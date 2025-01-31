#include "simdjson/simdjson.h"

namespace OCASI::GLTF {
    
    struct Json
    {
        simdjson::ondemand::parser Parser;
        simdjson::padded_string PaddedJsonString;
        simdjson::ondemand::document Json;
        
        simdjson::ondemand::document& Get() { return Json; }
    };
    
}

// The code is unnecessary with C++20:
#if !SIMDJSON_SUPPORTS_DESERIALIZATION
template <>
simdjson_inline simdjson::simdjson_result<float> simdjson::ondemand::value::get() noexcept
{
    double val;
    auto error = get_double().get(val);
    if (error)
        return error;
    return (float) val;
}
#endif
