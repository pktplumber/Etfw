
#pragma once

#include "Message.hpp"

#define MSG_STRUCT(modId, funcId)                       \
    public App::msg::BaseMsg<                                  \
        static_cast<App::msg::MsgModuleId_t>(modId),    \
        static_cast<App::msg::MsgFuncId_t>(funcId)      \
    >

#define NOOP_CMD(modId)     \
    App::msg::NoopCmd<                                  \
        static_cast<App::msg::MsgModuleId_t>(modId)     \
    >

#define MOD_ID      App::msg::MsgModuleId_t
#define FUNC_ID     App::msg::MsgFuncId_t
#define CMD_ID      FUNC_ID
