#ifdef OCASI_DEBUG

    #ifdef _WIN32

    #define OCASI_DEBUGBREAK() __debugbreak();

    #elif defined(__linux__)

    #include "signal.h"
    #define OCASI_DEBUGBREAK() raise(SIGTRAP);

    #endif

#define OCASI_ASSERT(x) if(!(x)) { OCASI_LOG_ERROR("Assertion failed at line {0} in file {1}.", __LINE__, Path(__FILE__).string()); OCASI_DEBUGBREAK(); }
#define OCASI_ASSERT_MSG(x, msg) if(!(x)) { OCASI_LOG_ERROR("Assertion failed: {}", msg);  OCASI_DEBUGBREAK(); }

#define OCASI_FAIL(msg) OCASI_ASSERT_MSG(false, msg);

#else

#define OCASI_ASSERT(x)
#define OCASI_ASSERT_MSG(x, msg)

#define OCASI_FAIL(msg) OCASI_LOG_ERROR(msg)

#endif