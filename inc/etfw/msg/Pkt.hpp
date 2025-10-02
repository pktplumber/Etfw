
#pragma once

#include "Message.hpp"
#include <etl/reference_counted_object.h>
#include <etl/reference_counted_message.h>

namespace etfw::msg
{
    struct iPkt
    {
        MsgId_t MsgId;
        size_t BufSz;
    };

    class RefCount : public etl::reference_counter<std::atomic_int32_t>
    {
    public:
        using Base_t = etl::reference_counter<std::atomic_int32_t>;

        RefCount():
            Base_t()
        {}

        RefCount(int32_t init_val):
            Base_t()
        {
            set_reference_count(init_val);
        }
    };

    class MsgBufPool;

    class Buf : public etl::ireference_counted_message
    {
    public:
        using RefCount_t = RefCount;

        etl::imessage& get_message() override
        {
            return *static_cast<etl::imessage*>(data());
        }

        const etl::imessage& get_message() const override
        {
            return *static_cast<const etl::imessage*>(data());
        }

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

        etl::ireference_counter& get_reference_counter() override
        {
            return ref_count_;
        }

        const etl::ireference_counter& get_reference_counter() const override
        {
            return ref_count_;
        }

        void release() override;

        uint8_t* data_buf();

        const uint8_t* data_buf() const
        {
            return reinterpret_cast<const uint8_t*>(this+1);
        }

        void* data()
        {
            return (this+1);
        }

        const void* data() const
        {
            return (this+1);
        }

        size_t buf_size();

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
