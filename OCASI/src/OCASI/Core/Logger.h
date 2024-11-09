#pragma once

#include "OCASI/Core/Base.h"

#include "spdlog/spdlog.h"

namespace OCASI {
    class Logger 
    {
    public:
        static void Init();
        static void SetFileFormatPattern(const std::string& fileFormatName);
        static void ResetPattern();

        static std::shared_ptr<spdlog::logger> GetLogger() { return s_Logger; }
    private:
        static std::shared_ptr<spdlog::logger> s_Logger;    
    };
}

#define FORMAT(x, ...) fmt::format(x, __VA_ARGS__)

#define OCASI_LOG_TRACE(...) OCASI::Logger::GetLogger()->trace(__VA_ARGS__)
#define OCASI_LOG_DEBUG(...) OCASI::Logger::GetLogger()->debug(__VA_ARGS__)
#define OCASI_LOG_INFO(...) OCASI::Logger::GetLogger()->info(__VA_ARGS__)
#define OCASI_LOG_WARN(...) OCASI::Logger::GetLogger()->warn(__VA_ARGS__)
#define OCASI_LOG_ERROR(...) OCASI::Logger::GetLogger()->error(__VA_ARGS__)