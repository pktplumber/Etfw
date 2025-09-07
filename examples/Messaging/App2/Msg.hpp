
#pragma once

#include <etfw/msg/Message.hpp>
#include "Cfg.hpp"

namespace app2
{
    /// @brief Makes App 2 say hello
    struct SayHello : public etfw::msg::command<AppId, 0>
    {};

    /// @brief Starts a child task that prints the number of iterations
    struct StartChildTask : public etfw::msg::command<AppId, 1>
    {
        uint32_t NumIterations;
        uint32_t TimeMsBetweenIterations;
    };

    /// @brief Starts sending a special stats message every wakeup
    struct StartSendingSpecialMsg : public etfw::msg::command<AppId, 2>
    {};

    /// @brief Stops sending the special stats message
    struct StopSendingSpecialMsg : public etfw::msg::command<AppId, 2>
    {};


    /// @brief Periodic stats message
    struct StatsMsg : public etfw::msg::telemetry<AppId, 0>
    {
        uint32_t AppWakeupCount;
        bool ChildTaskActive;
        bool CurrentlySendingPeriodicMsg;

        /// @brief App 2 standard stats message constructor
        StatsMsg():
            AppWakeupCount(0),
            ChildTaskActive(false),
            CurrentlySendingPeriodicMsg(false)
        {}
    };

    /// @brief Special stats msg. Sent every wakeup once 
    ///        "StartSendingSpecialMsg" has been received.
    ///        Stop by sending StopSendingSpecialMsg.
    struct SpecialStatsMsg : public etfw::msg::telemetry<AppId, 1>
    {
        uint32_t NumTimesSpecialMsgSent;

        SpecialStatsMsg():
            NumTimesSpecialMsgSent(0)
        {}
    };
}
