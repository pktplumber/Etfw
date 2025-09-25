
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
            size_t AllocCount;
            size_t ReleaseCount;

            Stats(size_t num_items);

            Stats();

            inline bool mem_avail() const
            {
                return (ItemsAllocated < NumItems);
            }

            inline size_t items_avail() const
            {
                return (NumItems - ItemsAllocated);
            }
 
            Stats& operator++();
            Stats operator++(int);
            Stats& operator--();
            Stats operator--(int);
        };


        /// TODO: need to construct with allocator or something, No max_items arg
        MsgBufPool();
        MsgBufPool(size_t max_items);

        void release(const etl::ireference_counted_message& msg) override;

        void lock() override;

        void unlock() override;

        inline const Stats& stats() const { return stats_; }

        /// @brief Allocate and create message
        /// @tparam TMsg Message type
        /// @tparam ...TArgs TMsg constructor argument types
        /// @param ...args TMsg constructor arguments
        /// @return Allocated msg buffer. Nullptr if allocation failed
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

        /// @brief Allocate message buffer and copy input message
        /// @tparam TMsg Copied message type
        /// @param msg Message to copy
        /// @return Allocated msg buffer. Nullptr if allocation failed
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

        /// @brief Allocate a raw message buf of bytes "sz".
        ///     User is responsible for constructing the appropriate class
        /// @param sz Bytes to allocate
        /// @return Allocated msg buffer. Nullptr if allocation failed
        Buf* allocate(const size_t sz);

    private:
        Os::Mutex mut_;
        Stats stats_;

        void* allocate_raw(size_t sz, size_t alignment);
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
