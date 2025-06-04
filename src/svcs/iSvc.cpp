
#include "svcs/iSvc.hpp"

using namespace etfw;

iSvc::iSvc(SvcId_t id, Name_t& name):
    Id(id),
    Name(name),
    IsInit(false),
    IsStarted(false)
{}

iSvc::iSvc(SvcId_t id, const char* name):
    Id(id),
    Name(name),
    IsInit(false),
    IsStarted(false)
{}

iSvc::Status iSvc::init(void)
{
    Status err = Status::Code::ALREADY_INIT;
    if (!IsInit)
    {
        err = init_();
        if (err.success())
        {
            IsInit = true;
        }
    }
    return err;
}

iSvc::Status iSvc::start(void)
{
    if (!IsInit)
    {
        printf("App not init\n");
        return Status::Code::UNINIT_ERR;
    }

    if (IsStarted)
    {
        printf("App already started\n");
        return Status::Code::ALREADY_STARTED;
    }

    Status err = start_();
    if (err.success())
    {
        IsStarted = true;
    }

    return err;
}

iSvc::Status iSvc::stop(void)
{
    if (!IsInit)
    {
        return Status::Code::UNINIT_ERR;
    }

    if (!IsStarted)
    {
        return Status::Code::STOPPED;
    }

    Status err = stop_();
    if (err.success())
    {
        IsStarted = false;
    }

    return err;
}

iSvc::Status iSvc::cleanup(void)
{
    if (!IsInit)
    {
        return Status::Code::UNINIT_ERR;
    }

    if (IsStarted)
    {
        return Status::Code::ALREADY_STARTED;
    }

    Status err = cleanup_();
    if (err.success())
    {
        IsInit = false;
    }

    return err;
}

// TEmp
#include <array>
#include <string>
#include <cstring>

void iSvc::log(const LogLevel level, const char* format, ...)
{
    constexpr std::size_t USER_BUFFER_SIZE = 1024;

    std::array<char, USER_BUFFER_SIZE> userBuffer;

    va_list args;
    va_start(args, format);
    std::vsnprintf(userBuffer.data(), userBuffer.size(), format, args);
    va_end(args);

    std::array<char, USER_BUFFER_SIZE+32> m_buf;
    snprintf(m_buf.data(), m_buf.size(), "[%s]: %s", name_raw(), userBuffer.data());
    EtfLog::log(level, m_buf.data());
}

void iSvc::log(const char* format, ...)
{
    va_list args;
    log(LogLevel::INFO, format, args);
}


/// Registry

