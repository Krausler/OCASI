#include "Logger.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace OCASI {

    const std::string DEFAULT_PATTERN = "%^[%T] %n: %v%$";

    std::shared_ptr<spdlog::logger> Logger::s_Logger;

    void Logger::Init()
    {
		spdlog::set_pattern(DEFAULT_PATTERN);
        s_Logger = spdlog::stdout_color_mt("OCASI");
    }

    void Logger::SetFileFormatPattern(const std::string& fileFormatName)
    {
        spdlog::set_pattern(std::format("%^[%T] %n {}: %v%$", fileFormatName));
    }

    void Logger::ResetPattern()
    {
        spdlog::set_pattern(DEFAULT_PATTERN);
    }
}