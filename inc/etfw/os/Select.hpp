
#pragma once

#include <cstdint>
#include "OsTypes.hpp"

namespace Os {
    constexpr std::size_t MAX_SELECTABLE_OBJS = 32;

    class Select
    {
        public:
            using FdSet = Os::OsFd_t[MAX_SELECTABLE_OBJS];

            enum Status: int32_t
            {
                OP_OK       = 0,
                NOT_OPEN    = -1,
                TIMEOUT     = -2,
                SET_FULL    = -3,
                INVALID_FD  = -4
            };

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
