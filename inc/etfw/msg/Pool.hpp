
#pragma once

#include <etl/shared_message.h>
#include <etl/reference_counted_message_pool.h>
#include <etl/fixed_sized_memory_block_allocator.h>

/// TODO: place in #ifdef guards to check if using stdlib
#include <atomic>

#include <os/Mutex.hpp>
#include "Message.hpp"
#include "Pkt.hpp"

namespace etfw::msg
{
    using ref_counted_msg = pkt;

    class MsgBufPool : public etl::ireference_counted_message_pool
    {
    public:
        using Buf_t = Buf;

        struct Stats
        {
            const size_t NumItems;
            size_t ItemsAllocated;
            size_t WaterMark;

            Stats(size_t num_items);

            Stats();

            inline bool mem_avail() const
            {
                return (ItemsAllocated < NumItems);
            }

            inline size_t items_avail() const
            {
                return NumItems - ItemsAllocated;
            }

            Stats& operator++();

            Stats operator++(int);

            Stats& operator--();

            Stats operator--(int);
        };

        void release(const etl::ireference_counted_message& msg) override
        {
            lock();
            //if (msg.get_reference_counter().get_reference_count() == 0)
            //{
                ::operator delete(static_cast<void*>(const_cast<etl::ireference_counted_message*>(&msg)));
                --stats_;
            //}
            unlock();
        }

        template <typename TMsg, typename... TArgs>
        Buf* allocate(TArgs&&... args)
        {
            Buf* ret = nullptr;
            const size_t total_sz = sizeof(Buf)+sizeof(TMsg);

            lock();
            ret = static_cast<Buf*>(allocate_raw(total_sz, etl::alignment_of<Buf>::value));
            unlock();

            if (ret != nullptr)
            {
                new(ret) Buf(*this, sizeof(TMsg));
                new(ret->data()) TMsg(etl::forward<TArgs>(args)...);
            }

            return ret;
        }

        template <typename TMsg>
        Buf* allocate(const TMsg& msg)
        {
            Buf* ret = nullptr;
            const size_t total_sz = sizeof(Buf)+sizeof(TMsg);

            lock();
            ret = static_cast<Buf*>(allocate_raw(total_sz, etl::alignment_of<Buf>::value));
            unlock();

            if (ret != nullptr)
            {
                new(ret) Buf(msg, *this);
            }

            return ret;
        }

        template <typename TMsg>
        Buf* allocate()
        {
            Buf* ret = nullptr;
            const size_t total_sz = sizeof(Buf)+sizeof(TMsg);

            lock();
            ret = static_cast<Buf*>(allocate_raw(total_sz, etl::alignment_of<TMsg>::value));
            unlock();

            if (ret != nullptr)
            {
                new(ret) Buf(*this, sizeof(TMsg));
                // Construct message into buffer
                new(ret->data()) TMsg();
            }

            return ret;
        }

        Buf* allocate(const size_t sz)
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
    
        void lock() override
        {
            mut_.lock();
        }

        void unlock() override
        {
            mut_.unlock();
        }

        MsgBufPool()
        {
            auto stat = mut_.init();
            assert(stat.success() &&
                "Failed to initialize mutex");
        }

        MsgBufPool(size_t max_items):
            stats_(max_items)
        {
            auto stat = mut_.init();
            assert(stat.success() &&
                "Failed to initialize mutex");
        }

        inline Stats& stats() { return stats_; }

        inline const Stats& stats() const { return stats_; }

    private:
        Os::Mutex mut_;
        Stats stats_;

        void* allocate_raw(size_t sz, size_t alignment)
        {
            void* ret = nullptr;
            if (stats_.mem_avail())
            {
                ret = (void*)(new uint8_t[(sz+3) & ~3]);
                stats_++;
            }
            return ret;
        }
    };


    class Pool
    {
    public:
        using counter_t = std::atomic_uint32_t;
        template <typename TMsg>
        using MsgOut_t = etl::reference_counted_message<TMsg, counter_t>;

        struct Stats
        {
            const size_t NumItems;
            size_t ItemsAllocated;
            size_t WaterMark;

            Stats(size_t num_items);

            Stats();

            inline bool mem_avail() const
            {
                return (ItemsAllocated < NumItems);
            }

            inline size_t items_avail() const
            {
                return NumItems - ItemsAllocated;
            }

            Stats& operator++();

            Stats operator++(int);

            Stats& operator--();

            Stats operator--(int);
        };

        Pool();

        Pool(size_t max_items);

        void release(ref_counted_msg& msg);

        ref_counted_msg* acquire_raw(const size_t msg_sz);

        template <typename MsgT>
        pkt* acquire(const MsgT& msg)
        {
            static_assert(etl::is_base_of<etl::imessage, MsgT>::value,
                "MsgT must derive from etl::imessage");
            pkt* p = acquire_raw(sizeof(MsgT));
            if (p)
            {
                // Construct msg into packet data buffer
                new (p->data_buf()) MsgT(msg);
            }

            return p;
        }

        template <typename MsgT, typename... Args>
        pkt* acquire(Args&&... args)
        {
            static_assert(etl::is_base_of<etl::imessage, MsgT>::value,
                "MsgT must derive from message type");
            pkt* p = acquire_raw(sizeof(MsgT));
            if (p)
            {
                new (p->data_buf()) MsgT(etl::forward<Args>(args)...);
            }
            return p;
        }

        pkt* allocate_raw(const size_t sz);

        template <typename TMsg, typename... TArgs>
        MsgOut_t<TMsg>* allocate(TArgs&&... args)
        {
            static_assert(etl::is_base_of<etl::imessage, TMsg>::value,
                "Not a message type");
            using ret_t = MsgOut_t<TMsg>;

            ret_t* ret = acquire_raw(sizeof(ret_t));
            //if (ret)
            //{
            //    new()
            //}
        }

        void lock();

        void unlock();

        inline const Stats& stats() const { return stats_; }

    private:
        Os::Mutex mut_;
        Stats stats_;
    };

}
