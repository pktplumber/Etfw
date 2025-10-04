
#pragma once

#include "Message.hpp"
#include <etl/reference_counted_object.h>
#include <etl/reference_counted_message.h>

namespace etfw::msg
{
    /// @brief Message reference counter type
    class RefCount : public etl::reference_counter<std::atomic_int32_t>
    {
    public:
        /// @brief Using atomic_int32. TODO: add compile time checks for atomic support
        using Base_t = etl::reference_counter<std::atomic_int32_t>;

        /// @brief Default constructor. Sets reference count to 0.
        RefCount();

        /// @brief Construct with initial value
        /// @param init_val Initial reference count value
        RefCount(int32_t init_val);
    };

    /// @brief Forward declare pool for use by message buffer class
    class MsgBufPool;

    /// @brief Message buffer. Allocated from pool
    class Buf : public etl::ireference_counted_message
    {
    public:
        /// @brief Reference count type
        using RefCount_t = RefCount;

        /// @brief Gets the etl::imessage at the data field
        /// @return Message interface
        etl::imessage& get_message() override
        {
            return *static_cast<etl::imessage*>(data());
        }

        /// @brief Gets a const reference to the etl::imessage
        /// @return Message interface
        const etl::imessage& get_message() const override
        {
            return *static_cast<const etl::imessage*>(data());
        }

        /// @brief Converts the data field to the message type
        /// @warning Size of message must be <= the allocated msg size
        /// @tparam MsgT Message type
        /// @return Message object
        template <typename MsgT>
        MsgT& get_message_type()
        {
            static_assert(etl::is_base_of<iBaseMsg, MsgT>::value,
                "Message must be of type iBaseMsg");
            assert(sizeof(MsgT) <= msg_sz_ &&
                "Attempt to convert to message with invalid size");
            return *static_cast<MsgT*>(data());
        }

        template <typename MsgT>
        const MsgT& get_message_type() const
        {
            return *static_cast<MsgT*>(data());
        }

        /// @brief Get message reference counter
        /// @return Reference counter
        etl::ireference_counter& get_reference_counter() override
        {
            return ref_count_;
        }

        /// @brief Get const message reference counter
        /// @return Const reference counter
        const etl::ireference_counter& get_reference_counter() const override
        {
            return ref_count_;
        }

        /// @brief Return the buffer to it's owner pool.
        void release() override;

        uint8_t* data_buf();

        const uint8_t* data_buf() const
        {
            return reinterpret_cast<const uint8_t*>(this+1);
        }

        /// @brief Get the message buffer's data field
        /// @return Pointer to the message data
        void* data()
        {
            return (this+1);
        }

        /// @brief Get the message buffer's data field
        /// @return Const pointer to the message data
        const void* data() const
        {
            return (this+1);
        }

        /// @brief Get allocated buffer size
        /// @return Buffer size
        size_t buf_size() const;

        friend class MsgBufPool;

    private:
        Buf(MsgBufPool& owner, size_t buf_sz);

        template <typename TMsg, typename... TArgs>
        Buf(MsgBufPool& owner, TArgs&&... args):
            owner_(owner),
            ref_count_(1),
            msg_sz_(sizeof(TMsg))
        {
            // Construct at allocated buffer. Assumes buf has been allocated by pool
            new(data()) TMsg(etl::forward<TArgs>(args)...);
        }

        template <typename TMsg>
        Buf(const TMsg& msg, MsgBufPool& owner):
            owner_(owner),
            ref_count_(1),
            msg_sz_(sizeof(TMsg))
        {
            // Construct at allocated buffer. Assumes buf has been allocated by pool
            new(data()) TMsg(msg);
        }

        MsgBufPool& owner_;
        RefCount_t ref_count_;
        const size_t msg_sz_;
    };

    
}
