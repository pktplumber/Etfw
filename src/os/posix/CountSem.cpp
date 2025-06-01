
#include "os/CountSem.hpp"
#include "errno.h"
#include "iostream"

using namespace Os;

CountSem::CountSem()
{
    CountSemCount++;
}

CountSem::CountSem(const CountVal init_val)
{
    int err = sem_init(&_Sem, 0, static_cast<unsigned int>(init_val));
    assert(err == 0 && "Failed to initialize semaphore");
    IsInit = true;
}

CountSem::~CountSem()
{
    if (IsInit)
    {
        sem_destroy(&_Sem);
    }
}

CountSem::Status CountSem::give(void) noexcept
{
    if (!IsInit)
    {
        return CountSem::Status::UNINIT;
    }

    CountSem::Status status = OP_OK;
    int err = sem_post(&_Sem);
    if (err != 0)
    {
        status = ERR;
    }

    return status;
}

CountSem::Status CountSem::take(void) noexcept
{
    int32_t err = sem_trywait(&_Sem);
    if (err == 0)
    {
        return CountSem::Status::OP_OK;
    }

    return CountSem::Status::ERR;
}

CountSem::Status CountSem::take(const TimeMs_t time_ms) noexcept
{
    if (!IsInit)
    {
        return CountSem::Status::UNINIT;
    }

    timespec ts;
    /// NOTE: This is unsafe in real-time applications. Changing system time could result
    /// in erroneous behavior.
    clock_gettime(CLOCK_REALTIME, &ts);  // Get current time

    // Calculate the target time for the timeout (relative timeout)
    int secs = static_cast<int>(time_ms) / 1000;
    long nsecs = (static_cast<long>(time_ms) % 1000) * 1000000L;

    // Add the timeout to the current time (monotonic)
    ts.tv_sec += secs;
    ts.tv_nsec += nsecs;

    // Normalize the time in case the nanoseconds exceed 1 second
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += ts.tv_nsec / 1000000000L;   // Convert nanoseconds overflow into seconds
        ts.tv_nsec %= 1000000000L;                // Keep nanoseconds within valid range
    }

    int err = sem_timedwait(&_Sem, &ts);
    if (err == 0)
    {
        return CountSem::Status::OP_OK;
    }
    else
    {
        int _errno = errno;
        if (_errno == ETIMEDOUT)
        {
            return CountSem::Status::TIMEOUT;
        }
    }

    return CountSem::Status::ERR;
}

CountSem::Status CountSem::init(CountVal val) noexcept
{
    if (!IsInit)
    {
        int err = sem_init(&_Sem, 0, static_cast<unsigned int>(val));
        if (err == 0)
        {
            IsInit = true;
            return CountSem::Status::OP_OK;
        }
        return CountSem::Status::ERR;
    }
    
    return CountSem::Status::IS_INIT;
}
