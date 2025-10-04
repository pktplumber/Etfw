
#pragma once

#include "Cfg.hpp"
#include "MsgHandler.hpp"

namespace app2
{
    class App : public etfw::App<App, Cfg>
    {
    public:
        using Base_t = etfw::App<App, Cfg>;
        using Status = Base_t::Status;
        using RunState = Base_t::RunState;
        using WakeupPipe_t = etfw::msg::QueuedRouter<App, 1, WakeupMsg>;

        static constexpr uint32_t WakeupTimeoutMs = 1500;

        App():
            Base_t(),
            fw_proxy_(*this),
            wakeup_pipe_(*this),
            msg_handler_(fw_proxy_)
        {}

        Status app_init()
        {
            subscribe_cmd(wakeup_pipe_.subscription());
            return Status::Code::OK;
        }

        RunState run_loop()
        {
            WakeupPipe_t::DequeueStat stat = wakeup_pipe_.receive_msgs(
                WakeupTimeoutMs);
            if (stat == WakeupPipe_t::DequeueStat::TIMEOUT)
            {
                log(etfw::LogLevel::WARNING, "Wakeup timeout");
            }
            
            return RunState::OK;
        }

        Status app_cleanup()
        {
            return Status::Code::OK;
        }

        /// @brief Wakeup handler. Calls command processor
        /// @param[in] wakeup Unused wakeup message
        void receive(const WakeupMsg& wakeup)
        {
            log(etfw::LogLevel::DEBUG, "Woke up");
            msg_handler_.process_commands();
        }

    private:
        AppFwProxy fw_proxy_;
        WakeupPipe_t wakeup_pipe_;
        MsgHandler msg_handler_;
    };
}
