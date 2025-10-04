
#include "msg/Pool.hpp"
#include <cstring>

using namespace etfw::msg;

MsgBufPool::Stats::Stats(size_t num_items):
    NumItems(num_items),
    ItemsInUse(0),
    WaterMark(0),
    AllocCount(0),
    ReleaseCount(0)
{}

MsgBufPool::Stats& MsgBufPool::Stats::operator++()
{
    ++AllocCount;
    ++ItemsInUse;
    if (ItemsInUse > WaterMark)
    {
        ++WaterMark;
    }
    return *this;
}

MsgBufPool::Stats MsgBufPool::Stats::operator++(int)
{
    Stats tmp = *this;
    ++(*this);
    return tmp;
}

MsgBufPool::Stats& MsgBufPool::Stats::operator--()
{
    ++ReleaseCount;
    --ItemsInUse;
    return *this;
}

MsgBufPool::Stats MsgBufPool::Stats::operator--(int)
{
    Stats tmp = *this;
    --(*this);
    return tmp;
}

/// TODO: need to remove/replace with actual num items for stats
MsgBufPool::MsgBufPool():
    stats_(100)
{
    auto stat = mut_.init();
    assert(stat.success() &&
        "Failed to initialize mutex");
}

MsgBufPool::MsgBufPool(size_t max_items):
    stats_(max_items)
{
    auto stat = mut_.init();
    assert(stat.success() &&
        "Failed to initialize mutex");
}

Buf* MsgBufPool::allocate(const size_t sz)
{
    Buf* ret = nullptr;
    const size_t total_sz = sizeof(Buf) + sz;

    lock();
    ret = static_cast<Buf*>(allocate_raw(total_sz, etl::alignment_of<void*>::value));
    unlock();

    if (ret != nullptr)
    {
        new(ret) Buf(*this, sz);
    }

    return ret;
}

void MsgBufPool::release(const etl::ireference_counted_message& msg)
{
    lock();
    /// TODO: ensure ref count == 0 ???
    ::operator delete(static_cast<void*>(const_cast<etl::ireference_counted_message*>(&msg)));
    --stats_;
    unlock();
}

void MsgBufPool::release(Buf* buf)
{
    if (buf != nullptr)
    {
        lock();
        ::operator delete(static_cast<void*>(buf));
        --stats_;
        unlock();
    }
}

void MsgBufPool::lock()
{
    mut_.lock();
}

void MsgBufPool::unlock()
{
    mut_.unlock();
}

void* MsgBufPool::allocate_raw(size_t sz, size_t alignment)
{
    void* ret = nullptr;
    if (stats_.mem_avail())
    {
        ret = (void*)(new uint8_t[(sz+3) & ~3]);
        stats_++;
    }
    return ret;
}

