#include "Logger.h"

#include "spdlog/sinks/stdout_color_sinks.h"

namespace OCASI {

    spdlog::logger Logger::m_Logger;

    void Logger::Init()
    {
		spdlog::set_pattern("%^[%T] %n: %v%$");
        m_Logger = spdlog::stdout_color_mt("OCASI");
    }
}