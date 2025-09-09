
#pragma once

#include <etfw/svcs/App.hpp>
#include <etfw/msg/Router.hpp>
#include "Msg.hpp"
#include "Child.hpp"

namespace app2
{
    class MsgHandler
    {
    public:
        using AppProxy = etfw::iApp::AppFwProxy;

        MsgHandler(AppProxy& proxy):
            proxy_(proxy)
        {}

        void process_commands()
        {

        }

        void receive(const SayHello& cmd)
        {
            proxy_.log(etfw::LogLevel::INFO, "Hello!");
        }

        void receive(const StartChildTask& cmd)
        {
            proxy_.log(etfw::LogLevel::INFO, 
                "Starting child task for %d iterations",
                cmd.NumIterations);
            child_.set_task_args(cmd.NumIterations,
                cmd.TimeMsBetweenIterations);
            proxy_.start_child(child_);
        }

        void receive(const StartSendingSpecialMsg& cmd)
        {
            
        }

        void receive(const StopSendingSpecialMsg& cmd)
        {

        }

    private:
        AppProxy& proxy_;
        Child child_;
    };
}
