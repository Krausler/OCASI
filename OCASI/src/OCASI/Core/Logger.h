#pragma once

#include <functional>
#include <string>
#include <format>

namespace OCASI {
    
    enum class LogLevel
    {
        None = 0,
        Info,
        Warn,
        Error
    };
    
    /*! @brief The OCVK logging system.
     *
     *  The OCVK logger works by supplying the message and LogLevel to the user, who may handle logging as they may.
     *  If no custom logging function is supplied by the user, a default logging function is used, that prints the output
     *  to the console. In this case, LogLevel::Info messages have a colour of blue, LogLevel::Warn yellow and
     *  LogLevel::Error the colour red.
     */
    class Logger
    {
    public:
        //! Function pointer template for user defined logging function.
        using LoggerFunc = std::function<void(LogLevel, const std::string&)>;
    public:
        /*! @brief Sets a user defined logging function to handle message output.
         *
         *  @param func The function to be called for logging
         */
        static void SetLoggerFunc(LoggerFunc& func) { s_LoggerFunction = func; }
        /*! @brief Sets the logging prefix. A log message is structured like this:
         *         'OCVK <loggerName/nothing>: <message>'
         *
         *  @param loggerName The string inserted after 'OCVK'
         */
        static void SetLoggerName(const std::string& loggerName);
        //! @brief Resets the logging prefix to the static default value 'OCVK'.
        static void ResetLoggerName();
        
        /*! @brief Calls the user supplied logging function with the log level and the log message
         *
         *  @param level The LogLevel of the message
         *  @param msg The to be logged message without prefix.
         */
        static void Log(LogLevel level, const std::string& msg);
        
        /*! @brief Logs with LogLevel::Info
         *
         *  @param message The message to be logged
         */
        static void LogInfo(const std::string& message)
        {
            Log(LogLevel::Info, message);
        }
        
        /*! @brief Logs with LogLevel::Warn
         *
         *  @param message The message to be logged
         */
        static void LogWarn(const std::string& message)
        {
            Log(LogLevel::Warn, message);
        }
        
        /*! @brief Logs with LogLevel::Error
         *
         *  @param message The message to be logged
         */
        static void LogError(const std::string& message)
        {
            Log(LogLevel::Error, message);
        }
        
        static const std::string& GetLoggerPrefix() { return s_LoggerPrefix; }
    
    private:
        static std::string s_LoggerPrefix;
        static LoggerFunc s_LoggerFunction;
    };
}

#define FORMAT(x, ...) std::format(x, __VA_ARGS__)

#define OCASI_LOG_INFO(x) OCASI::Logger::LogInfo(x)
#define OCASI_LOG_WARN(x) OCASI::Logger::LogWarn(x)
#define OCASI_LOG_ERROR(x) OCASI::Logger::LogError(x)