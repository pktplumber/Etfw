
#pragma once

#include "OsTypes.hpp"
#include "etfw_assert.hpp"
#include "status.hpp"

namespace Os {
    class Mutex
    {
        public:

            struct StatusTrait
            {
                enum class Code : int32_t
                {
                    OK,
                    OS_INIT_ERR,
                    LOCK_FAILURE,
                    TIMEOUT,
                    UNLOCK_FAILURE
                };

                static constexpr StatusStr_t ErrStrLkup[] =
                {
                    "Success",
                    "OS initialization error",
                    "OS Lock failure",
                    "Unlock failure"
                };
            };

            using Status = EtfwStatus<StatusTrait>;

            Mutex();

            Status init();

            Status lock();

            Status lock(const TimeMs_t t_ms);

            Status unlock();

            Status destroy();

        private:
            MutexHandle_t mutex_;
    };
}
