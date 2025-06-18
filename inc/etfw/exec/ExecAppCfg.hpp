
#pragma once

#include "etfw/svcs/SvcCfg.hpp"

namespace etfw {
namespace exec {

    constexpr SvcId_t ExecAppId = 0;

    template <uint8_t TPriority, size_t TStackSz>
    struct ExecAppCfg : public SvcCfg<
        ExecAppId, 
        ActiveSvcCfg<TPriority, TStackSz>
    >
    {
        static constexpr char* NAME = "EXEC";
    };
}
}
