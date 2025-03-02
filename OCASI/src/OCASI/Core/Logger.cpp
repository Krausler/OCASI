#include "Logger.h"

#include <OCASI/Core/Base.h>

#include <iostream>

namespace OCASI {
    
    // VT100 colour codes
    const std::string_view BLUE = "\x1b[34m";
    const std::string_view YELLOW = "\x1b[33m";
    const std::string_view RED = "\x1b[31m";
    
    void DefaultLoggerFunction(LogLevel level, const std::string& msg)
    {
        switch (level) {
            case LogLevel::Info:
                std::cout << BLUE << msg << std::endl;
                break;
            case LogLevel::Warn:
                std::cout << YELLOW << msg << std::endl;
                break;
            case LogLevel::Error:
                std::cout << RED << msg << std::endl;
                break;
            default:
                std::cout << "Invalid LogLevel" << std::endl;
        }
    }
    
    const std::string_view STATIC_PREFIX = "OCVK";
    
    std::string Logger::s_LoggerPrefix = std::string(STATIC_PREFIX);
    Logger::LoggerFunc Logger::s_LoggerFunction = DefaultLoggerFunction;
    
    void Logger::SetLoggerName(const std::string& loggerName)
    {
        s_LoggerPrefix = FORMAT("{} {}", STATIC_PREFIX, loggerName);
    }
    
    void Logger::ResetLoggerName()
    {
        s_LoggerPrefix = STATIC_PREFIX;
    }
    
    void Logger::Log(LogLevel level, const std::string& msg)
    {
        if (!s_LoggerFunction)
        {
            std::cout << "OCVK: No logger function. This should not happen!" << std::endl;
            return;
        }
        
        s_LoggerFunction(level, std::format("{}: {}", s_LoggerPrefix, msg));
    }
}