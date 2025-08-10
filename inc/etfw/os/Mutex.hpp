
#pragma once

#include "OsTypes.hpp"
#include "etfw_assert.hpp"

namespace Os {
    class Mutex
    {
        public:
            enum class Status : int32_t
            {
                OK,
                OS_INIT_ERR,
                LOCK_FAILURE,
                TIMEOUT,
                UNLOCK_FAILURE
            };

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
