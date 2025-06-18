
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

            /// @brief Constructor. Initializes semaphore to 0
            CountSem();

            /// @brief Constructor. Initializes semaphore to init_val
            /// @param init_val Count value to initialize
            CountSem(const CountVal init_val);

            /// @brief Deconstructor
            ~CountSem();

            /// @brief 
            /// @param  
            /// @return 
            Status give() noexcept;

            Status take() noexcept;

            Status take(const TimeMs_t time_ms) noexcept;

            Status init(const CountVal val) noexcept;

            Status init() noexcept { return init(0); }

            static size_t CountSemCount;

        private:
            sem_t _Sem;
            bool IsInit;
    };

    inline size_t CountSem::CountSemCount = 0;
}
