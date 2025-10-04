
#pragma once

#include "Cfg.hpp"
#include <etfw/msg/Message.hpp>

namespace app1
{
    namespace msg
    {
        // ~~~~~~~~~~~~ Message Codes ~~~~~~~~~~~~
        constexpr etfw::msg::FuncId_t SayHelloCode = 0;
        constexpr etfw::msg::FuncId_t PrintThisStrCode = 1;
        constexpr etfw::msg::FuncId_t ReturnResponseCode = 2;

        constexpr etfw::msg::FuncId_t GeneralStatsCode = 0;
        constexpr etfw::msg::FuncId_t OtherStatsCode = 1;

        // ~~~~~~~~~~~~ Command types ~~~~~~~~~~~~

        template <etfw::msg::FuncId_t FuncIdV>
        using Cmd_t = etfw::msg::command<AppId, FuncIdV>;

        struct SayHello : public Cmd_t<SayHelloCode>
        {};

        struct PrintThisString : public Cmd_t<PrintThisStrCode>
        {
            etl::string<20> StrToPrint;
        };

        struct ReturnResponse : public Cmd_t<ReturnResponseCode>
        {
            bool ExecOk;
        };

        // ~~~~~~~~~~~~ Status msg types ~~~~~~~~~~~~~~

        template <etfw::msg::FuncId_t FuncIdV>
        using Tlm_t = etfw::msg::telemetry<AppId, FuncIdV>;

        struct Stats : public Tlm_t<GeneralStatsCode>
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

        struct OtherStats : public Tlm_t<OtherStatsCode>
        {
            uint32_t RandInt1;
            uint32_t RandInt2;

            OtherStats():
                RandInt1(0),
                RandInt2(0)
            {}
        };
    }
}
