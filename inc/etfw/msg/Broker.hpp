
#pragma once

#include <etl/message_broker.h>
#include "Message.hpp"
#include "Pool.hpp"
#include "Pipe.hpp"
#include <os/Mutex.hpp>

#ifndef MSG_MAX_NUM_SUBSCRIPTIONS
#define MSG_MAX_NUM_SUBSCRIPTIONS   64
#endif

namespace etfw::msg {

    /// @brief Message type identifier
    using MsgId = etl::message_id_t;

    /// @brief List of message IDs. TODO: needs to be static type
    using MsgIdContainer = std::vector<MsgId>;

    /// @brief Broker subscription. Defines messages a router is subscribed to
    class Subscription : public etl::message_broker::subscription
    {
    public:
        using Base_t = etl::message_broker::subscription;
        using MsgSpan_t = etl::message_broker::message_id_span_t;

        /// @brief Construct a msg subscription with an initializer list
        /// @param module Router/pipe/module subscribed to the msg ids
        /// @param ids Message IDs the router is subscribed to
        Subscription(
            etl::imessage_router& module,
            std::initializer_list<MsgId> ids
        ):
            Base_t(module),
            IdList(ids)
        {}

        /// @brief Construct a subscription from a message ID list
        /// @param module The router owning the subscription
        /// @param msg_ids Message IDs the router is subscribed to
        Subscription(
            etl::imessage_router& module,
            MsgIdContainer &msg_ids
        ):
            Base_t(module),
            IdList(msg_ids)
        {}

        /// @brief Construct empty subscription object
        /// @param pipe The pipe/router owning the subscription
        Subscription(
            etl::imessage_router& pipe
        ):
            Base_t(pipe)
        {}

        /// @brief Construct subscription from static message types
        /// @tparam ...TMsgs Message types. Must derive from iBaseMsg
        /// @param[in] pipe Subscribed pipe
        template <typename... TMsgs>
        Subscription(
            etl::imessage_router& pipe
        ):
            Base_t(pipe)
        {
            static_assert((etl::is_base_of<iBaseMsg, TMsgs>::value && ...),
                "Types must derive from iBaseMsg");
            (IdList.emplace_back(static_cast<const iBaseMsg&>(TMsgs()).get_message_id()), ...);
        }

        template <typename... TMsgs>
        Subscription(
            etl::imessage_router& pipe,
            TMsgs&&... msgs
        ):
            Base_t(pipe)
        {
            static_assert((etl::is_base_of<iBaseMsg, TMsgs>::value && ...),
                "Types must derive from iBaseMsg");
            (IdList.emplace_back(static_cast<const iBaseMsg&>(TMsgs()).get_message_id()), ...);
        }

        /// @brief Returns a view of the subscribed message IDs
        /// @return Message ID view
        MsgSpan_t message_id_list() const override
        {
            return MsgSpan_t(IdList.begin(), IdList.end());
        }

        bool subscribe(const MsgId id)
        {
            IdList.push_back(id);
            return true;
        }

        bool unsubscribe(const MsgId id)
        {
            IdList.erase(std::remove(IdList.begin(), IdList.end(), id), IdList.end());
            return true;
        }

        bool is_subscribed(const MsgId id) const
        {
            for (auto& id_: IdList)
            {
                if (id_ == id)
                {
                    return true;
                }
            }
            return false;
        }

        bool has(const MsgId id) const
        {
            for (auto& id_: IdList)
            {
                if (id_ == id)
                {
                    return true;
                }
            }
            return false;
        }

    private:
        /// TODO: replace with static container [MSG_MAX_NUM_SUBSCRIPTIONS]
        MsgIdContainer IdList;
    };

    /// @brief Message broker class
    //using Broker = etl::message_broker;

    class SharedMsg : public etl::shared_message
    {
    public:
        using Base_t = etl::shared_message;
        using Base_t::Base_t;

        template <typename TPool>
        static SharedMsg create_from_size(TPool& owner, size_t size)
        {
            etl::ireference_counted_message* p_msg = owner.allocate_raw(size);
            assert(p_msg != nullptr && "Failed to allocate message");
            if (p_msg != nullptr)
            {
                p_msg->get_reference_counter().set_reference_count(1U);
            }

            return SharedMsg(*p_msg);
        }
    };



    class Broker : etl::message_broker
    {
    public:
        struct Stats
        {
            size_t RegisteredPipes;
            size_t NumSendCalls;
            size_t AllocateFailures;

            Stats();
        };

        using Base_t = etl::message_broker;

        /// TODO: un-expose these methods after refactor
        using Base_t::receive;
        using Base_t::subscribe;

        Broker();

        void send(const iMsg& msg, const size_t msg_sz);

        /// @brief Send copy of message
        /// @tparam TMsg 
        /// @param msg 
        template <typename TMsg>
        void send(const TMsg& msg)
        {
            static_assert(etl::is_base_of<iBaseMsg, TMsg>::value,
                "Message type must derive from etfw::msg::imsg");
            Buf* buf = msg_pool_.allocate<TMsg>(msg);
            if (buf != nullptr)
            {
                send_buf(*buf);
            }
            else
            {
                // failed to allocate message buffer
                stats_.AllocateFailures++;
            }
        }

        template <typename TMsg, typename... TArgs>
        void send(TArgs&&... args)
        {
            static_assert(etl::is_base_of<iBaseMsg, TMsg>::value,
                "Message type must derive from iMsg");
            Buf* buf = msg_pool_.allocate<TMsg>(args...);
            if (buf != nullptr)
            {
                send_buf(*buf);
            }
            else
            {
                // failed to allocate message buffer
                stats_.AllocateFailures++;
            }
        }

        /// @brief Send a pre-allocated message buffer
        /// @warning Buffer cannot be used after this is called.
        /// @details This method routes a pre-allocated message buffer (from
        ///     "get_message_buf" to the appropriate pipes. The used is
        ///     responsible for copying a valid message into the buffer's
        ///     data field. If the message does not have any subscribers, the
        ///     buffer is returned to the pool.
        /// @param[in] msg_buf Message buffer to send
        void send_buf(Buf& msg_buf);

        /// @brief Register a pipe
        /// @param pipe 
        void register_pipe(iPipe& pipe);

        void unregister_pipe(iPipe& pipe);

        /// @brief Get the internal memory pool statistics
        /// @return Message buffer pool statistics
        inline const MsgBufPool::Stats& pool_stats() const
        { return msg_pool_.stats(); }

        /// @brief Get the broker's statistics
        /// @return Broker's internal statistics
        inline const Stats& stats() const { return stats_; }

        /// @brief Get a message buffer. Allows for zero-copy routing.
        /// @warning User is responsible for memory management. If a buffer is
        ///     acquired and unused, it must be release via 
        ///     "return_message_buf". The user must copy a valid message into
        ///     the buffer before sending via the "send_buf" method. Once the
        ///     buffer is sent, it is no longer considered valid and should not
        ///     be used.
        /// @param[in] buf_sz Size of the buffer to allocate
        /// @return Pointer to message buffer class. Nullptr on failure.
        Buf* get_message_buf(const size_t buf_sz);

        void return_message_buf(Buf* buf);

    private:
        MsgBufPool msg_pool_;
        Os::Mutex lock_;
        Stats stats_;
    };


    class iSubscription
    {
    public:
        using Base_t = etl::message_broker::subscription;
        using MsgSpan_t = etl::message_broker::message_id_span_t;


    };

}
