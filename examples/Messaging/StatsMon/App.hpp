
#pragma once

#include "Cfg.hpp"
#include "StatsHandler.hpp"
#include "Msg.hpp"

namespace stats_mon
{
    struct Cfg : public ActiveAppCfg_t<AppId>
    {
        static constexpr const char* NAME = "STATS_MON";
    };

    class App : public etfw::App<App, Cfg>
    {
    public:
        using Base_t = etfw::App<App, Cfg>;
        using Base_t::Status;
        using Base_t::RunState;
        using WakeupPipe_t = etfw::msg::QueuedWakeupPipe<App, AppId>;
        using WakeupMsg_t = etfw::msg::wakeup_msg<AppId>;

        static constexpr uint32_t WakeupTimeoutMs = 1500;

        App(MsgTbl& msg_tbl):
            Base_t(),
            fw_proxy_(*this),
            stats_msg_handler_(fw_proxy_),
            wakeup_pipe_(*this)
        {
            msg_tbl.add_observer(stats_msg_handler_);
        }

        Status app_init()
        {
            comms.register_pipe(wakeup_pipe_);
            return Status::Code::OK;
        }

        RunState run_loop()
        {
            wakeup_pipe_.wait(WakeupTimeoutMs);
            return RunState::OK;
        }

        Status app_cleanup()
        {
            return Status::Code::OK;
        }

        void on_wakeup()
        {
            log(etfw::LogLevel::DEBUG, "Woke up");
            stats_msg_handler_.process_stats_messages();
        }

    private:
        AppFwProxy fw_proxy_;
        StatsHandler stats_msg_handler_;
        WakeupPipe_t wakeup_pipe_;
    };
}
