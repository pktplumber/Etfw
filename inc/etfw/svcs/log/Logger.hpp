
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

namespace etfw {
    enum class LogLevel
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    template <typename... TLogWriters>
    struct Logger
    {
        using Level = LogLevel;
        static constexpr size_t MsgBufSz = ETF_MAX_LOG_MSG_SZ;

        static void log(const Level level, const char* format, ...)
        {
            std::array<char, MsgBufSz> MsgBuf;
            va_list args;
            va_start(args, format);
            std::vsnprintf(MsgBuf.data(), MsgBuf.size(), format, args);
            va_end(args);

            (TLogWriters::write(level, MsgBuf.data()), ...);
        }
    };
    
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
};

#ifndef EtfLog
#define EtfLog    ::etfw::Logger<ConsoleLogPolicy>
#endif
