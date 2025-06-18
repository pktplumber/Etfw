
#pragma once

#include "etfw/svcs/App.hpp"
#include "ExecAppCfg.hpp"

/// @brief ExecApp priority is configurable, but should
///        be higher priority than any other running tasks
#ifndef EXEC_APP_PRIORITY
#define EXEC_APP_PRIORITY   1
#endif

/// @brief ExecApp task stack size. Left configureable to
///        account for differences between platforms
#ifndef EXEC_APP_STACK_SZ
#define EXEC_APP_STACK_SZ   2048
#endif

namespace etfw {
namespace exec {

    template <typename... Apps>
    class ExecApp : public App<
        ExecApp,
        ExecAppCfg<EXEC_APP_PRIORITY, EXEC_APP_STACK_SZ>
    >
    {
        public:
            
    };
}
}
