
#pragma once

#include <cstdint>
#define POSIX_COMPLIANT_OS

#ifdef POSIX_COMPLIANT_OS
#include <semaphore.h>
#endif


namespace Os {

#ifdef POSIX_COMPLIANT_OS
    using OsFd_t = int;
    constexpr OsFd_t OS_INVALID_FD = -1;
    constexpr OsFd_t OS_MIN_FD = 0;
    using TimeMs_t = uint32_t;
    typedef sem_t SemHandle_t;
    typedef pthread_mutex_t MutexHandle_t;
#endif // POSIX_COMPLIANT_OS

    class OsObj
    {
        public:
            OsObj(): Fd(OS_INVALID_FD) {}

        protected:
            OsFd_t Fd;
    };
}
