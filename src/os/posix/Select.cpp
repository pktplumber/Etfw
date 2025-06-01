
#include "os/Select.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

using namespace Os;

static void clear_fds(Select::FdSet &fds)
{
    for (auto &fd: fds)
    {
        fd = 0;
    }
}

inline void _update_max_fd(OsFd_t fd, OsFd_t &max)
{
    if (fd > max)
    {
        max = fd;
    }
}

void add_set(fd_set &set, Select::FdSet &set_in, size_t num_fds, Os::OsFd_t &max_fd)
{
    for (size_t i = 0; i < num_fds; i++)
    {
        if (set_in[i] != Os::OS_INVALID_FD)
        {
            FD_SET(set_in[i], &set);
            _update_max_fd(set_in[i], max_fd);
        }
    }
}

Select::Select():
    RdSetIdx(0),
    MaxFd(Os::OS_INVALID_FD)
{}

Select::~Select() = default;


Select::Status Select::add_fd(Os::OsFd_t fd)
{
    if (RdSetIdx >= MAX_SELECTABLE_OBJS)
    {
        return Select::Status::SET_FULL;
    }
    RdSet[RdSetIdx] = fd;
    RdSetIdx++;

    return Select::Status::OP_OK;
}

Select::Status Select::read_wait(TimeMs_t ms, FdSet &fds, std::size_t &num_fds)
{
    size_t fds_idx = 0;
    struct timeval timeout;
    int secs = ms/1000;
    int usecs = (ms%1000)*1000;
    timeout.tv_sec = secs;
    timeout.tv_usec = usecs;

    clear_fds(fds);
    fd_set rd_set;
    FD_ZERO(&rd_set);
    add_set(rd_set, RdSet, RdSetIdx, MaxFd);
    Select::Status status = TIMEOUT;
    int err = select(MaxFd+1, &rd_set, nullptr, nullptr, &timeout);
    if (err > 0)
    {
        // IO success
        status = OP_OK;
        for (auto &fd: RdSet)
        {
            if (FD_ISSET(fd, &rd_set))
            {
                fds[fds_idx] = fd;
                fds_idx++;
            }
        }
        num_fds = fds_idx;
    }
    else if (err < 0)
    {
        status = INVALID_FD;
    }
    return status;
}

void Select::clear_set(void)
{
    for (auto &fd: RdSet)
    {
        fd = Os::OS_INVALID_FD;
    }
    RdSetIdx = 0;
    MaxFd = -1;
}
