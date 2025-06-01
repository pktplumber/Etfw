
#pragma once

#include "OsTypes.hpp"
#include <cassert>

namespace Os
{
    class CountSem
    {
        public:
            enum Status : int32_t
            {
                OP_OK = 0,
                TIMEOUT = -1,
                UNINIT = -2,
                IS_INIT = -3,
                ERR = -4
            };

            using CountVal = uint32_t;

            CountSem();

            CountSem(const CountVal init_val);

            ~CountSem();

            Status give(void) noexcept;

            Status take(void) noexcept;

            Status take(const TimeMs_t time_ms) noexcept;

            Status init(CountVal val) noexcept;

            Status init(void) noexcept { return init(0); }

            static size_t CountSemCount;

        private:
            sem_t _Sem;
            bool IsInit;
    };

    inline size_t CountSem::CountSemCount = 0;
}
