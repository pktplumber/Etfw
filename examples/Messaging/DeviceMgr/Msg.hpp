
#pragma once

#include <etfw/msg/Message.hpp>
#include "Cfg.hpp"

namespace dev_mgr::cmds
{
    struct TurnOn : public etfw::msg::command_msg<
        TurnOn, AppId, 0>
    {};

    struct TurnOff : public etfw::msg::command_msg<
        TurnOn, AppId, 1>
    {};

    struct SetPowerLvl : public etfw::msg::command_msg<
        TurnOn, AppId, 2>
    {
        uint16_t PowerLvl;

        SetPowerLvl(uint16_t lvl):
            PowerLvl(lvl)
        {}
    };
}
