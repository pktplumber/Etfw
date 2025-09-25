
#include "msg/Pool.hpp"
#include <cstring>

using namespace etfw::msg;

MsgBufPool::Stats::Stats(size_t num_items):
    NumItems(num_items),
    ItemsAllocated(0),
    WaterMark(0),
    AllocCount(0),
    ReleaseCount(0)
{}

MsgBufPool::Stats::Stats():
    NumItems(100),
    ItemsAllocated(0),
    WaterMark(0),
    AllocCount(0),
    ReleaseCount(0)
{}

MsgBufPool::Stats& MsgBufPool::Stats::operator++()
{
    ++AllocCount;
    ++ItemsAllocated;
    if (ItemsAllocated > WaterMark)
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
    --ItemsAllocated;
    return *this;
}

MsgBufPool::Stats MsgBufPool::Stats::operator--(int)
{
    Stats tmp = *this;
    --(*this);
    return tmp;
}

MsgBufPool::MsgBufPool()
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


Pool::Stats::Stats(size_t num_items):
    NumItems(num_items),
    ItemsAllocated(0),
    WaterMark(0)
{}

Pool::Stats::Stats():
    NumItems(100),
    ItemsAllocated(0),
    WaterMark(0)
{}

Pool::Stats& Pool::Stats::operator++()
{
    ++ItemsAllocated;
    if (ItemsAllocated > WaterMark)
    {
        ++WaterMark;
    }
    return *this;
}

Pool::Stats Pool::Stats::operator++(int)
{
    Stats tmp = *this;
    ++(*this);
    return tmp;
}

Pool::Stats& Pool::Stats::operator--()
{
    --ItemsAllocated;
    return *this;
}

Pool::Stats Pool::Stats::operator--(int)
{
    Stats tmp = *this;
    --(*this);
    return tmp;
}


Pool::Pool()
{
    auto stat = mut_.init();
    assert(stat.success() &&
        "Failed to initialize pool mutex");
}

Pool::Pool(size_t max_items):
    stats_(max_items)
{
    auto stat = mut_.init();
    assert(stat.success() &&
        "Failed to initialize pool mutex");
}

ref_counted_msg* Pool::acquire_raw(const size_t msg_sz)
{
    ref_counted_msg* ret = nullptr;
    lock();
    if (stats_.mem_avail())
    {
        uint8_t* mem = new uint8_t[sizeof(ref_counted_msg) + msg_sz];
        if (mem != nullptr)
        {
            ret = new (mem) ref_counted_msg(*this, msg_sz);
            ++stats_;
        }
    }
    unlock();
    return ret;
}

pkt* Pool::allocate_raw(const size_t sz)
{
    pkt* ret = nullptr;
    lock();
    if (stats_.mem_avail())
    {
        uint8_t* mem = new uint8_t[sz];
        if (mem != nullptr)
        {
            ret = new(mem) pkt(*this, sz);
        }
    }
    return ret;
}

//template <typename MsgT>
//pkt* Pool::acquire(const MsgT& msg)
//{
//    static_assert(etl::is_base_of<etl::imessage, MsgT>::value,
//        "MsgT must derive from etl::imessage");
//    pkt* p = acquire_raw(sizeof(MsgT));
//    if (p)
//    {
//        // Construct msg into packet data buffer
//        new (p->data_buf()) MsgT(msg);
//    }
//
//    return p;
//}

void Pool::release(ref_counted_msg& msg)
{
    lock();
    int32_t val = msg.ref_count().decrement_reference_count();
    if (val == 0)
    {
        delete[] reinterpret_cast<uint8_t*>(&msg);
        --stats_;
    }
    unlock();
}

void Pool::lock()
{
    mut_.lock();
}

void Pool::unlock()
{
    mut_.unlock();
}
