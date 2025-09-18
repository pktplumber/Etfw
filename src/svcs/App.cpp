
#include "svcs/App.hpp"
#include <cassert>
#include "svcs/log/Logger.hpp"

using namespace etfw;

using Status = iApp::Status;

msg::Broker iApp::CmdBroker;
msg::Broker iApp::StatusBroker;
msg::Broker iApp::WakeupBroker;

Status iApp::register_child(iSvc& child)
{
    Status stat = Status::Code::OK;

    if (!Children.is_registered(child))
    {
        if (!Children.register_svc(child).success())
        {
            stat = Status::Code::REGISTRY_FULL;
        }
    }
    else
    {
        stat = Status::Code::REREGISTRATION_ERR;
    }

    return stat;
}

Status iApp::start_child(iSvc& child)
{
    Status stat = register_child(child);

    if (stat.success())
    {
        stat = child.start();
        if (stat.success())
        {
            log(EtfLog::Level::INFO, 
                "Started child service %s (ID = %d)",
                child.name_raw(), child.id());
        }
    }

    return stat;
}

Status iApp::unregister_all_children()
{
    return Status::Code::UNKNOWN_ERR;
}

void iApp::send_cmd(const etl::imessage& msg)
{
    CmdBroker.receive(msg);
}

void iApp::subscribe_msg(msg::Subscription& subscription)
{
    CmdBroker.subscribe(subscription);
}

void iApp::subscribe_cmd(msg::Subscription &msgs)
{
    CmdBroker.subscribe(msgs);
}

void iApp::subscribe_cmd(etl::imessage_router& handler,
    std::initializer_list<msg::MsgId> msg_ids)
{
    msg::Subscription subscription(handler, msg_ids);
    subscribe_cmd(subscription);
}

void iApp::subscribe_cmd(etl::imessage_router& handler,
    msg::MsgIdContainer &msg_ids)
{
    msg::Subscription subscription(handler, msg_ids);
    subscribe_cmd(subscription);
}

void iApp::subscribe_status(msg::Subscription &msgs)
{
    StatusBroker.subscribe(msgs);
}

void iApp::subscribe_status(etl::imessage_router& handler,
    std::initializer_list<msg::MsgId> msg_ids)
{
    msg::Subscription subscription(handler, msg_ids);
    subscribe_status(subscription);
}

void iApp::subscribe_status(etl::imessage_router& handler,
    msg::MsgIdContainer &msg_ids)
{
    msg::Subscription subscription(handler, msg_ids);
    subscribe_status(subscription);
}


// ~~~~~~~~ AppFwProxy method definitions ~~~~~~~~

iApp::AppFwProxy::AppFwProxy(iApp& app):
    app_(app)
{}

iApp::AppFwProxy::AppFwProxy(iApp* app):
    app_(*app)
{
    ETFW_ASSERT(app != nullptr,
        "Attempt to construct app proxy with nullptr");
}

Status iApp::AppFwProxy::register_child(iSvc& svc)
{
    return app_.register_child(svc);
}

Status iApp::AppFwProxy::start_child(iSvc& svc)
{
    return app_.start_child(svc);
}

void iApp::AppFwProxy::subscribe(msg::Subscription& subscription)
{
    iApp::subscribe_msg(subscription);
}

void iApp::AppFwProxy::log(const LogLevel level, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    app_.log(level, format, args);
    va_end(args);
}
