
#pragma once

#include "OsTypes.hpp"
#include "etfw_assert.hpp"
#include "status.hpp"

namespace Os {
    class Mutex
    {
    public:

        /// @brief Mutex return code trait
        struct StatusTrait
        {
            /// @brief Return codes
            enum class Code : int32_t
            {
                OK,
                OS_INIT_ERR,
                LOCK_FAILURE,
                TIMEOUT,
                UNLOCK_FAILURE
            };

            /// @brief Return code string representation
            static constexpr StatusStr_t ErrStrLkup[] =
            {
                "Success",
                "OS initialization error",
                "OS Lock failure",
                "Unlock failure"
            };
        };

        /// @brief Return code type
        using Status = EtfwStatus<StatusTrait>;

        Mutex();

        /// @brief Initialize the mutex. Must be called before use.
        /// @return Initialization status
        Status init();

        /// @brief Lock the mutex.
        /// @return Lock status
        Status lock();

        /// @brief Lock the mutex. If already locked, wait for t_ms.
        /// @param t_ms Milliseconds to wait for lock until timeout.
        /// @return Lock status
        Status lock(const TimeMs_t t_ms);

        /// @brief Unlock the mutex
        /// @return Unlock status
        Status unlock();

        /// @brief Destroy or "de-init" the mutex. Returns resources to OS.
        /// @return Destroy status
        Status destroy();

    private:
        MutexHandle_t mutex_;
    };
}
