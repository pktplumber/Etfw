
#include "svcs/Executor.hpp"
#include "svcs/log/Logger.hpp"

using namespace etfw;

using Status = iExecutor::Status;

#define __LOG(lvl, fmt, args)   EtfLog::log(lvl, "EXEC", fmt, args)

iExecutor::Node_t iExecutor::find(const SvcId_t id)
{
    iExecutor::Node_t ret = nullptr;

    for (auto& app: Apps)
    {
        if (app->id() == id)
        {
            ret = app;
            break;
        }
    }

    return ret;
}

bool iExecutor::is_registered(const SvcId_t id)
{
    if (find(id) != nullptr)
    {
        return true;
    }
    return false;
}

bool iExecutor::is_registered(const iApp& app)
{
    if (find(app.id()) != nullptr)
    {
        return true;
    }
    return false;
}

Status iExecutor::register_app(iApp& app)
{
    Status stat = Status::Code::OK;
    size_t num_registered = Apps.size();
    if (num_registered < ContainerSize)
    {
        if (!is_registered(app))
        {
            Apps.push_front(&app);
            // Need some sort of check for push_back
            // in case exceptions/asserts are disabled
            if (Apps.size() == num_registered+1)
            {
                //printf("Registered app %X\n", app);
                stat = Status::Code::OK;
            }
            else
            {
                // Unknown registration error. Unlikely to happen 
                // but still need to cover our bases
                stat = Status::Code::UNKNOWN_REGISTRATION_ERR;
            }
        }
        else
        {
            stat = Status::Code::ID_TAKEN;
        }
    }
    else
    {
        stat = Status::Code::REGISTRY_FULL;
    }

    return stat;
}

Status iExecutor::register_app(iApp* app)
{
    Status stat = Status::Code::OK;
    size_t num_registered = Apps.size();
    if (num_registered < ContainerSize)
    {
        if (!is_registered(*app))
        {
            Apps.push_front(app);
            // Need some sort of check for push_back
            // in case exceptions/asserts are disabled
            if (Apps.size() == num_registered+1)
            {
                stat = Status::Code::OK;
            }
            else
            {
                // Unknown registration error. Unlikely to happen 
                // but still need to cover our bases
                stat = Status::Code::UNKNOWN_REGISTRATION_ERR;
            }
        }
        else
        {
            stat = Status::Code::ID_TAKEN;
        }
    }
    else
    {
        stat = Status::Code::REGISTRY_FULL;
    }

    return stat;
}

Status iExecutor::start_svc(iApp* app)
{
    Status stat;
    iSvc::Status svc_stat = app->start();
    if (svc_stat.success())
    {
        stat = Status::Code::OK;
    }
    else
    {
        stat = Status::Code::START_FAILURE;
    }

    return stat;
}

Status iExecutor::start_all()
{
    Status stat = Status::Code::OK;
    for (auto &app: Apps)
    {
        if (!app->is_init())
        {
            iSvc::Status svc_stat = app->init();
            if (!svc_stat.success())
            {
                continue;
            }
        }

        if (!app->is_started())
        {
            iSvc::Status svc_stat = app->start();
            if (!svc_stat.success())
            {
                continue;
            }
            else
            {
                // Log and increment counters
            }
        }
    }

    return stat;
}

Status iExecutor::start(const SvcId_t app_id)
{
    Status stat;
    iApp* app = find(app_id);
    if (app != nullptr)
    {
        if (!app->is_started())
        {
            if (!app->is_init())
            {
                iSvc::Status svc_stat = app->init();
                if (svc_stat.success())
                {
                    EtfLog::log(LogLevel::INFO,
                        "EXEC",
                        "%s app (ID = %d) initialized",
                        app->name_raw(), app->id());
                    stat = start_svc(app);
                }
                else
                {
                    stat = Status::Code::INITIALIZATION_ERR;
                }
            }
            else
            {
                stat = start_svc(app);
            }
        }
        else
        {
            stat = Status::Code::SVC_ALREADY_STARTED;
        }
    }
    else
    {
        stat = Status::Code::UNKNOWN_ID;
    }

    return stat;
}

Status iExecutor::stop_all()
{
    for (auto& app: Apps)
    {
        if (app->is_started())
        {
            app->stop();
        }
    }

    return Status::Code::OK;
}

Status iExecutor::stop(const SvcId_t app_id)
{
    Status stat = Status::Code::OK;
    iApp* app = find(app_id);
    if (app != nullptr)
    {
        if (app->is_started())
        {
            iSvc::Status svc_stat = app->stop();
            if (svc_stat.success())
            {
                EtfLog::log(LogLevel::INFO,
                    "EXEC",
                    "Stopped %s app. (ID = %d)",
                    app->name_raw(), app->id());
            }
            else
            {
                EtfLog::log(LogLevel::ERROR,
                    "EXEC",
                    "App stop failure. %s app (ID = %d). %s",
                    app->name_raw(), app->id(), svc_stat.str());
            }
        }
        else
        {
            EtfLog::log(LogLevel::INFO,
                "EXEC",
                "App already stopped. %s (ID = %d) is not running",
                app->name_raw(), app->id());
        }
    }
    else
    {
        EtfLog::log(LogLevel::ERROR,
                    "EXEC",
                    "App stop failure. %s app (ID = %d) is not registered",
                    app->name_raw(), app->id());
    }

    return stat;
}
