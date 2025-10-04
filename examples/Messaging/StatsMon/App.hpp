
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
        using WakeupPipe_t = etfw::msg::QueuedRouter<App, 1, WakeupMsg>;

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
            subscribe_cmd(wakeup_pipe_.subscription());
            return Status::Code::OK;
        }

        RunState run_loop()
        {
            WakeupPipe_t::DequeueStat stat = wakeup_pipe_.receive_msgs(
                WakeupTimeoutMs
            );
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

        void receive(const WakeupMsg& wakeup)
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
