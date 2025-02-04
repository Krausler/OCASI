#include "Logger.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace OCASI {

    const std::string DEFAULT_PATTERN = "%^[%T] %n: %v%$";

    std::shared_ptr<spdlog::logger> Logger::s_Logger;
    std::string Logger::s_CurrentPattern;

    void Logger::Init()
    {
		spdlog::set_pattern(DEFAULT_PATTERN);
        s_CurrentPattern = "OCASI";
        s_Logger = spdlog::stdout_color_mt(s_CurrentPattern);
    }

    void Logger::SetFileFormatPattern(const std::string& fileFormatName)
    {
        s_CurrentPattern = fileFormatName;
        spdlog::set_pattern(fmt::format("%^[%T] %n {}: %v%$", s_CurrentPattern));
    }

    void Logger::ResetPattern()
    {
        spdlog::set_pattern(DEFAULT_PATTERN);
    }
    
    std::shared_ptr<spdlog::logger> Logger::GetLogger()
    {
        if (!s_Logger)
            Init();
        return s_Logger;
    }
    
    const std::string& Logger::GetPattern()
    {
        return s_CurrentPattern;
    }
}