
#pragma once

#include "Common.hpp"

namespace app2
{
    constexpr etfw::SvcId_t AppId = 2;

    constexpr size_t CmdPipeDepth = 5;

    struct Cfg : public ActiveAppCfg_t<AppId>
    {
        static constexpr const char* NAME = "EX_APP_2";
    };
}
