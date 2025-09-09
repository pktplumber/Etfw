
#pragma once

#include "Common.hpp"
#include "Cfg.hpp"
#include "Msg.hpp"
#include <etfw/msg/Router.hpp>

namespace app1
{
    struct Cfg : public ActiveAppCfg_t<AppId>
    {
        static constexpr const char* NAME = "EX_APP_1";
    };

    class App : public etfw::App<App, Cfg>
    {
        public:
            using Base_t = etfw::App<App, Cfg>;
            using Status = Base_t::Status;
            using RunState = Base_t::RunState;

            App():
                Base_t(),
                cmd_pipe(*this)
            {}

            Status app_init()
            {
                subscribe_cmd(cmd_pipe.subscription());
                return Status::Code::OK;
            }

            RunState run_loop()
            {
                cmd_pipe.process_msg_queue(1000);
                log(etfw::LogLevel::INFO, "Processed command queue");
                return RunState::OK;
            }

            Status app_cleanup()
            {
                return Status::Code::OK;
            }

            void receive(const msg::SayHello& cmd)
            {
                log(etfw::LogLevel::INFO, "Hello!");
            }

            void receive(const msg::PrintThisString& cmd)
            {
                log(etfw::LogLevel::INFO, "Cmd string = %s",
                    cmd.StrToPrint.data());
            }

            void receive(const msg::ReturnResponse& cmd)
            {
                if (cmd.ExecOk)
                {

                }
            }

        private:
            msg::Stats stats_;
            etfw::msg::QueuedRouter<App, CmdPipeDepth,
                msg::SayHello,
                msg::PrintThisString,
                msg::ReturnResponse> cmd_pipe;
    };
}
