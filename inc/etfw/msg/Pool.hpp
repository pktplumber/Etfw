
#pragma once

#include <etl/reference_counted_message_pool.h>

/// TODO: place in #ifdef guards to check if using stdlib
#include <atomic>

#include <os/Mutex.hpp>
#include "Message.hpp"
#include "Pkt.hpp"

namespace etfw::msg
{
    /// @brief Message buffer pool
    class MsgBufPool : public etl::ireference_counted_message_pool
    {
    public:
        using Buf_t = Buf;

        /// @brief Message buffer pool statistics
        struct Stats
        {
            const size_t NumItems;  //< Number of items allocated in the pool
            size_t ItemsInUse;      //< Items currently in use/unfreed
            size_t WaterMark;       //< Highest number of items in use at once
            size_t AllocCount;      //< Count of items successfully allocated
            size_t ReleaseCount;    //< Count of items successfully released

            /// @brief Construct stats with NumItems set
            /// @param num_items Number of items in the pool
            Stats(size_t num_items);

            /// @brief Checks if any items are available to be allocated
            /// @return True if items can be allocated. False if the pool is
            ///     depleted.
            inline bool mem_avail() const
            {
                return (ItemsInUse < NumItems);
            }

            /// @brief Calculates the number of items available
            /// @return Number of items available for allocation
            inline size_t items_avail() const
            {
                return (NumItems - ItemsInUse);
            }
 
            // Custom operators
            Stats& operator++();
            Stats operator++(int);
            Stats& operator--();
            Stats operator--(int);
        };


        /// TODO: need to construct with allocator or something, No max_items arg
        MsgBufPool();
        MsgBufPool(size_t max_items);

        /// @brief Release from a reference counted message
        /// @param msg Message to release
        void release(const etl::ireference_counted_message& msg) override;

        /// @brief Release a raw memory buffer. Buffer must have been
        ///     allocated from this pool
        /// @param[in] buf Buffer to release
        void release(Buf* buf);

        /// @brief Get the pool statistics
        /// @return Const reference to the pool statistics
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
        ///     User is responsible for copying the appropriate class
        ///     into the buffer. 
        /// @param sz Bytes to allocate
        /// @return Allocated msg buffer. Nullptr if allocation failed
        Buf* allocate(const size_t sz);

    private:
        Os::Mutex mut_;
        Stats stats_;

        /// @brief Allocates the raw memory buffer
        /// @param[in] sz Size to allocate
        /// @param[in] alignment Buffer alignment
        /// @return Raw memory buffer. Null if pool is depleted
        void* allocate_raw(size_t sz, size_t alignment);

        /// @brief Lock the pool. Must be called before allocation or release.
        void lock() override;

        /// @brief Unlock the pool. Must be called after allocation or release.
        void unlock() override;
    };
}
