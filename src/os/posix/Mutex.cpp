
#include "os/Mutex.hpp"
#include <pthread.h>
#include <errno.h>

using namespace Os;

Mutex::Mutex()
{}

Mutex::Status Mutex::init()
{
    Mutex::Status stat = Status::OK;
    int os_ret = pthread_mutex_init(&mutex_, nullptr);
    if (os_ret != 0)
    {
        stat = Status::OS_INIT_ERR;
    }
    return stat;
}

Mutex::Status Mutex::lock()
{
    Mutex::Status stat = Status::OK;
    int os_ret = pthread_mutex_lock(&mutex_);
    if (os_ret != 0)
    {
        stat = Status::LOCK_FAILURE;
    }
    return stat;
}

Mutex::Status Mutex::lock(const TimeMs_t t_ms)
{
    Mutex::Status stat = Status::OK;

    timespec ts;
    /// NOTE: This is unsafe in real-time applications. Changing system time could result
    /// in erroneous behavior.
    clock_gettime(CLOCK_REALTIME, &ts);  // Get current time

    // Calculate the target time for the timeout (relative timeout)
    int secs = static_cast<int>(t_ms) / 1000;
    long nsecs = (static_cast<long>(t_ms) % 1000) * 1000000L;

    // Add the timeout to the current time (monotonic)
    ts.tv_sec += secs;
    ts.tv_nsec += nsecs;

    // Normalize the time in case the nanoseconds exceed 1 second
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += ts.tv_nsec / 1000000000L;   // Convert nanoseconds overflow into seconds
        ts.tv_nsec %= 1000000000L;                // Keep nanoseconds within valid range
    }

    int os_ret = pthread_mutex_timedlock(&mutex_, &ts);
    if (os_ret == ETIMEDOUT)
    {
        stat = Status::TIMEOUT;
    }
    else if (os_ret != 0)
    {
        stat = Status::LOCK_FAILURE;
    }

    return stat;
}

Mutex::Status Mutex::unlock()
{
    Mutex::Status stat = Status::OK;
    int os_ret = pthread_mutex_unlock(&mutex_);
    if (os_ret != 0)
    {
        stat = Status::UNLOCK_FAILURE;
    }
    return stat;
}
