
#pragma once

#include "Cfg.hpp"
#include <etfw/svcs/msg/Message.hpp>

namespace app1
{
    namespace msg
    {
        // ~~~~~~~~~~~~ Command types ~~~~~~~~~~~~~~

        template <etfw::msg::FuncId_t FuncIdV>
        using Cmd_t = etfw::msg::command<AppId, FuncIdV>;

        struct SayHello : public Cmd_t<0>
        {};

        struct PrintThisString : public Cmd_t<1>
        {
            etl::string<20> StrToPrint;
        };

        struct ReturnResponse : public Cmd_t<2>
        {
            bool ExecOk;
        };

        // ~~~~~~~~~~~~ Status msg types ~~~~~~~~~~~~~~

        template <etfw::msg::FuncId_t FuncIdV>
        using Tlm_t = etfw::msg::telemetry<AppId, FuncIdV>;

        struct Stats : public Tlm_t<0>
        {
            uint32_t AppExecCounter;
            bool OddCount;

            Stats():
                AppExecCounter(0),
                OddCount(false)
            {}

            Stats& operator++()
            {
                AppExecCounter++;
                OddCount = !OddCount;
                return *this;
            }
        };
    }
}
