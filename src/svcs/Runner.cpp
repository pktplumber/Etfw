
#include "svcs/Runner.hpp"
#include "svcs/iSvc.hpp"

using namespace etfw;

iSvcRunner::iSvcRunner():
    Svc(nullptr),
    State(State_t::CREATED)
{}

iSvcRunner::iSvcRunner(iSvc* svc):
    Svc(svc),
    State(State_t::CREATED)
{}

iSvcRunner::~iSvcRunner() = default;

void iSvcRunner::stop_children()
{
    if (Svc != nullptr)
    {
        iSvc::iRegistry *children = Svc->children();
        if (children != nullptr)
        {
            for (size_t i = 0; i < children->size(); i++)
            {
                children->data()[i]->stop();
            }
        }
    }
}

void iSvcRunner::log(const LogLevel lvl, const char* fmt, ...)
{
    va_list args;
    if (Svc != nullptr)
    {
        Svc->log(lvl, fmt, args);
    }
}

PassiveRunner::PassiveRunner(iSvc* svc):
    iSvcRunner(svc)
{}

iSvcRunner::RunStatus PassiveRunner::start()
{
    log(LogLevel::INFO, "Passive service started");
    Svc->pre_run_init();
    return iSvcRunner::RunStatus::OK;
}

iSvcRunner::RunStatus PassiveRunner::stop()
{
    stop_children();
    /// TODO: need some sort of wait/async call back for children to stop
    /// before calling cleanup
    Svc->post_run_cleanup();
    log(LogLevel::INFO, "Passive service stopped");
    return iSvcRunner::RunStatus::OK;
}
