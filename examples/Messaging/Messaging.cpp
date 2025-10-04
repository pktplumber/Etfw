/**
 * @file Messaging.cpp
 * @author your name (you@domain.com)
 * @brief Example of app messaging
 * @version 0.1
 * @date 2025-09-05
 * 
 * @copyright Copyright (c) 2025
 * 
 * @details In this example, 
 * 
 */

#include "etfw/svcs/App.hpp"
#include <etfw/msg/Pipe.hpp>

#include "App1/ActiveApp1.hpp"
#include "App2/App2.hpp"
#include "StatsMon/App.hpp"

#include "MsgScheduler.hpp"

#include <etfw/msg/Pool.hpp>

template <etfw::SvcId_t TAppId>
using ActiveAppCfg_t = etfw::SvcCfg<TAppId, etfw::ActiveSvcCfg<AppPriority, AppStackSz>>;

// ~~~~~~~~~~~~~~~ App 3 configuration ~~~~~~~~~~~~~~~

static constexpr etfw::SvcId_t App3Id = 3;

/// @brief Passive application configuration
struct ActiveApp3Cfg : public ActiveAppCfg_t<App3Id>
{
    static constexpr const char* NAME = "EX_APP_3";
};

stats_mon::MsgTbl StatsMonMsgTbl{
    stats_mon::MsgTbl::Entry(app1::msg::Stats::ID),
    stats_mon::MsgTbl::Entry(app2::StatsMsg::ID),
    stats_mon::MsgTbl::Entry(app2::SpecialStatsMsg::ID),
};

#define APP2_WAKEUP         15   // 1.5 s
#define STATS_MON_WAKEUP    2    // 200 ms

#define APP1_GEN_STATS_PERIOD   20

int main()
{
    app1::App app1;
    app2::App app2;
    stats_mon::App stats_mon_app(StatsMonMsgTbl);
    MsgScheduler msg_scheduler{
        {app2::WakeupMsg::ID, APP2_WAKEUP},
        {stats_mon::WakeupMsg::ID, STATS_MON_WAKEUP},
        {etfw::msg::telemetry_request<app1::AppId, app1::msg::GeneralStatsCode>::ID, APP1_GEN_STATS_PERIOD}
        //MsgScheduler::Entry(etfw::msg::telemetry_request<app1::AppId, app1::msg::GeneralStatsCode>)
    };

    printf("App1 init status: %s\n", app1.init().str());
    printf("App2 init status: %s\n", app2.init().str());
    printf("Stats mon app init status: %s\n", stats_mon_app.init().str());

    app1.start();
    app2.start();
    stats_mon_app.start();

    int iters = 0;

    while (true)
    {
        msg_scheduler.run();
        iters++;
        if (iters == 30)
        {
            printf("\n\nUpdating tbl\n\n");
            StatsMonMsgTbl.add_id(1);
        }
    }

    return 0;
}
