
#pragma once

#include "Message.hpp"

#define MSG_STRUCT(modId, funcId)                       \
    public App::Msg::BaseMsg<                                  \
        static_cast<App::Msg::MsgModuleId_t>(modId),    \
        static_cast<App::Msg::MsgFuncId_t>(funcId)      \
    >

#define NOOP_CMD(modId)     \
    App::Msg::NoopCmd<                                  \
        static_cast<App::Msg::MsgModuleId_t>(modId)     \
    >

#define MOD_ID      App::Msg::MsgModuleId_t
#define FUNC_ID     App::Msg::MsgFuncId_t
#define CMD_ID      FUNC_ID
