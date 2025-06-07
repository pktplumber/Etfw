
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

void iSvc::log(const LogLevel level, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    EtfLog::log(level, name_raw(), format, args);
    va_end(args);
}
