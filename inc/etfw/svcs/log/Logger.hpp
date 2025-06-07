
#pragma once

#include <etl/string.h>

// temp
#include <cstdio>
#include <cstdarg>
#include <array>

#ifndef ETF_MAX_LOG_MSG_SZ
#define ETF_MAX_LOG_MSG_SZ  256
#endif

#define ETF_LOG_MSG_TS_SZ   12

#define ETF_LOG_FMT     "%-20s %-12s %s"

namespace etfw {

    /// @brief Log severity levels
    enum class LogLevel
    {
        DEBUG,      ///< Low severity message for debugging/diagnostics.
        INFO,       ///< Indicates a nominal event/operation has occurred.
        WARNING,    ///< Medium severity message indicating a warning.
        ERROR,      ///< High severity, off-nominal event has occurred.
        CRITICAL    ///< Highest severity, unrecoverable error has occurred.
    };

    /// @brief Returns the string representation of a log severity level
    /// @param lvl Severity level
    /// @return C-style string of severity level
    static constexpr const char* log_lvl_to_str(const LogLevel lvl)
    {
        switch (lvl)
        {
        case LogLevel::DEBUG:
            return "DEBUG";
            break;
        
        case LogLevel::INFO:
            return "INFO";
            break;
        
        case LogLevel::WARNING:
            return "WARNING";
            break;
        
        case LogLevel::ERROR:
            return "ERROR";
            break;
        
        case LogLevel::CRITICAL:
            return "CRITICAL";
            break;
        
        default:
            return "UNKNOWN";
            break;
        }
    }

    template <typename... TLogWriters>
    struct Logger
    {
        using Level = LogLevel;
        static constexpr size_t MsgBufSz = ETF_MAX_LOG_MSG_SZ;

        /// @brief Sends a C-style string to all writers
        /// @param level Severity level
        /// @param format String to write. Formattable
        /// @param Args Variadic string format arguments 
        static void log(const Level level, const char* format, ...)
        {
            std::array<char, MsgBufSz> MsgBuf;
            va_list args;
            va_start(args, format);
            std::vsnprintf(MsgBuf.data(), MsgBuf.size(), format, args);
            va_end(args);

            (TLogWriters::write(level, MsgBuf.data()), ...);
        }

        /// @brief Sends a C-style string with caller information to all writers.
        /// @param level Severity level
        /// @param caller_name Calling component's name 
        /// @param fmt String to write. Formattable
        /// @param Args Variadic string format arguments 
        static void log(const Level level, const char* caller_name, const char* fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            (TLogWriters::write_new(level, caller_name, fmt, args), ...);
            va_end(args);
        }

        /// @brief Sends a C-style string with caller information to all writers.
        /// @param level Severity level
        /// @param caller_name Calling component's name 
        /// @param fmt String to write. Formattable
        /// @param args Pre-processed va_list string format arguments
        static void log(const Level level, const char* caller_name, const char* fmt, va_list args)
        {
            (TLogWriters::write_new(level, caller_name, fmt, args), ...);
        }

        /// @brief Returns the string representation of a log severity level
        /// @param lvl Severity level
        /// @return C-style string of severity level
        static constexpr const char* to_str(const LogLevel lvl)
        {
            return log_lvl_to_str(lvl);
        }
    };

    /// @brief Default logging policy. Does not output anything, so it
    ///        can be used in systems where logging is not supported.
    struct NullLogPolicy
    {
        static void write(const LogLevel level, const char* msg) {}
    };
}

struct ConsoleLogPolicy
{
    static void write(const etfw::LogLevel level, const char* msg)
    {
        printf("%s\n", msg);
    }

    static void write_new(const etfw::LogLevel lvl, const char* caller, const char* fmt, va_list fmt_args)
    {

        std::array<char, 128> user_msg{};
        vsnprintf(user_msg.data(), user_msg.size(), fmt, fmt_args);
        
        std::array<char, 256> msg_output;
        snprintf(msg_output.data(), msg_output.size(),
            ETF_LOG_FMT, caller, log_lvl_to_str(lvl), user_msg.data());
        printf("%s\n", msg_output.data());
    }
};

#ifndef EtfLog
#define EtfLog    ::etfw::Logger<ConsoleLogPolicy>
#endif
