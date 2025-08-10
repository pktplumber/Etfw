
#pragma once

#include <cstdint>
#include "OsTypes.hpp"
#include "etfw/status.hpp"

namespace Os {
    constexpr std::size_t MAX_SELECTABLE_OBJS = 32;

    class Select
    {
        public:
            using FdSet = Os::OsFd_t[MAX_SELECTABLE_OBJS];

            struct StatusTrait
            {
                enum class Code : int32_t
                {
                    OK,
                    TIMEOUT,
                    SET_FULL,
                    INVALID_FD
                };

                static constexpr StatusStr_t ErrStrLkup[] =
                {
                    "Success",
                    "Timeout",
                    "Set is full. Cannot add more fds",
                    "One or more file descriptors is invalid",
                };
            };

            using Status = EtfwStatus<StatusTrait>;

            Select();
            ~Select();

            Status add_fd(Os::OsFd_t fd);

            Status read_wait(Os::TimeMs_t ms, FdSet &fds, std::size_t &num_fds);

            void clear_set(void);

        private:
            FdSet RdSet;
            std::size_t RdSetIdx;
            Os::OsFd_t MaxFd;
    };
}
