
#include "svcs/App.hpp"
#include <cassert>
#include "svcs/log/Logger.hpp"

using namespace etfw;

using Status = iApp::Status;

Msg::Broker iApp::CmdBroker;
Msg::Broker iApp::StatusBroker;
Msg::Broker iApp::WakeupBroker;

Status iApp::start_child(iSvc& child)
{
    Status stat = Status::Code::OK;

    if (!Children.is_registered(child))
    {
        if (Children.register_svc(child).success())
        {
            stat = child.start();
            if (stat.success())
            {
                log(EtfLog::Level::INFO, 
                    "Started child service %d (%s)",
                    child.id(), child.name_raw());
            }
        }
        else
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

void iApp::send_cmd(const etl::imessage& msg)
{
    CmdBroker.receive(msg);
}

void iApp::subscribe_cmd(Msg::Subscription &msgs)
{
    CmdBroker.subscribe(msgs);
}

void iApp::subscribe_cmd(etl::imessage_router& handler,
    std::initializer_list<Msg::MsgId> msg_ids)
{
    Msg::Subscription subscription(handler, msg_ids);
    subscribe_cmd(subscription);
}

void iApp::subscribe_cmd(etl::imessage_router& handler,
    Msg::MsgIdContainer &msg_ids)
{
    Msg::Subscription subscription(handler, msg_ids);
    subscribe_cmd(subscription);
}

void iApp::subscribe_status(Msg::Subscription &msgs)
{
    StatusBroker.subscribe(msgs);
}

void iApp::subscribe_status(etl::imessage_router& handler,
    std::initializer_list<Msg::MsgId> msg_ids)
{
    Msg::Subscription subscription(handler, msg_ids);
    subscribe_status(subscription);
}

void iApp::subscribe_status(etl::imessage_router& handler,
    Msg::MsgIdContainer &msg_ids)
{
    Msg::Subscription subscription(handler, msg_ids);
    subscribe_status(subscription);
}
