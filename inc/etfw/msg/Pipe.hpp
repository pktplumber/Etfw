
#pragma once

#include "Subscription.hpp"
#include "BlockingMsgQueue.hpp"
#include "Pkt.hpp"
#include <etl/message_router.h>

namespace etfw::msg
{
    /// @brief Message pipe interface. Router with message subscription
    class iPipe : public etl::imessage_router
    {
    public:
        using Base_t = etl::imessage_router;
        using Subscription_t = subscription;
        using PipeId_t = etl::message_router_id_t;
        static constexpr PipeId_t DefaultPipeId = 0;

        bool is_null_router() const override { return false; }
        bool is_producer() const override { return false; }
        bool is_consumer() const override { return true; }

        /// @brief Checks if the pipe is subscribed to the input msg id
        /// @param id Message ID to check
        /// @return True if subscribed. Otherwise, false.
        bool accepts(etl::message_id_t id) const override
        {
            return subbed_msgs_.has(id);
        }

        /// @brief Checks if the pipe is subscribed to the message.
        /// @param msg Message to check
        /// @return True if subscribed. Otherwise, false.
        bool accepts(const etl::imessage& msg) const
        {
            return accepts(msg.get_message_id());
        }

        /// @brief Checks if the pipe is subscribed to input message type.
        /// @tparam TMsg Message type
        /// @return True if subscribed. Otherwise, false.
        template <typename TMsg>
        bool accepts() const
        {
            static_assert(etl::is_base_of<etl::imessage, TMsg>::value,
                "TMsg must derive from etl::imessage");
            return accepts(TMsg::ID);
        }

        /// @brief Get the message subscription
        /// @return Const reference to the subscription
        inline const Subscription_t& subs() const { return subbed_msgs_; }

        /// @brief Get the message subscription
        /// @return Reference to the message subscription
        inline Subscription_t& subs() { return subbed_msgs_; }

    protected:
        /// @brief Default constructor. Builds empty message subscription
        iPipe():
            Base_t(DefaultPipeId),
            subbed_msgs_(*this)
        {}

        /// @brief Construct pipe with custom ID an empty msg subscription
        /// @param id Pipe ID
        iPipe(PipeId_t id):
            Base_t(id),
            subbed_msgs_(*this)
        {}

        /// @brief Construct pipe with custom ID and known msg subscription
        /// @param id Pipe ID
        /// @param msg_ids Message IDs to subscribe to
        iPipe(
            PipeId_t id,
            std::initializer_list<MsgId_t> msg_ids
        ):
            Base_t(id),
            subbed_msgs_(*this, msg_ids)
        {}

        template <typename... TMsgs>
        iPipe():
            Base_t(DefaultPipeId),
            subbed_msgs_(*this, TMsgs{}...)
        {}

        template <typename... TMsgs>
        iPipe(PipeId_t id):
            Base_t(id),
            subbed_msgs_(*this, TMsgs{}...)
        {}

        /// @brief Subscribed messages
        Subscription_t subbed_msgs_;
    };

    template <typename THandler>
    class Pipe : public iPipe
    {
    public:
        using Base_t = iPipe;

        Pipe(THandler& handler):
            Base_t(),
            handler_(handler)
        {}

        Pipe(PipeId_t id, THandler& handler):
            Base_t(id),
            handler_(handler)
        {}

        /// @brief Handles a message sent to this pipe. 
        /// @details Overrides etl::imessage_router (base of iPipe) 
        ///          "receive" pure virtual method.
        /// @param[in] msg Msg to handle
        virtual void receive(const etl::imessage& msg) override
        {
            handler_.handle(msg);
        }

    private:
        THandler& handler_;
    };

    /// @brief 
    /// @tparam THandler The message handler class type 
    /// @tparam QueueDepth Maximum number of messages that can be queued
    ///         at one time.
    template <typename THandler, size_t QueueDepth>
    class QueuedPipe : public Pipe<THandler>
    {
    public:
        using Base_t = iPipe;
        using PipeId_t = typename Base_t::PipeId_t;
        using Base_t::accepts;

        QueuedPipe(THandler& handler):
            Base_t(handler)
        {}

        QueuedPipe(PipeId_t id, THandler& handler):
            Base_t(id, handler)
        {}

    private:
        BlockingMsgQueue<iPkt*, QueueDepth> queue_;
    };

    /// @brief Synchronous pipe with static message subscription.
    ///        Message subscription is determined at compile time.
    template <typename THandler, typename... TMsgs>
    class StaticPipe : public Pipe<THandler>
    {
    public:
        using Base_t = Pipe<THandler>;
        using PipeId_t = typename Base_t::PipeId_t;
        using Base_t::receive;

        StaticPipe(THandler& handler):
            Base_t(handler)
        {}

        StaticPipe(PipeId_t id, THandler& handler):
            Base_t(id, handler)
        {}

        void receive(const etl::imessage& msg) override
        {

        }

    private:
        using This_t = StaticPipe<THandler, TMsgs...>;
    };

    /// @brief Queued pipe with static message subscription. Message
    ///        subscription is determined at compile time
    /// @details The StaticQueuedPipe is used for receiving asynchronous messages
    ///          copied directly into the pipe (i.e. no zero copy). This should
    ///          be used for handling commands or status messages that will not
    ///          change throughout the lifetime of the program that do not
    ///          require high performance.
    template <typename THandler, size_t QueueDepth, typename... TMsgs>
    class StaticQueuedPipe : public QueuedPipe<THandler, QueueDepth>
    {
    public:
        using Base_t = QueuedPipe<THandler, QueueDepth>;
        using PipeId_t = typename Base_t::PipeId_t;
        using Base_t::accepts;

        StaticQueuedPipe(THandler& handler):
            Base_t(handler)
        {}

        StaticQueuedPipe(PipeId_t id, THandler& handler):
            Base_t(id, handler)
        {}

        void receive(const etl::imessage& msg) override
        {
            if (accepts(msg))
            {
                queue_.emplace(msg);
            }
        }

        using Stat = bool;

        Stat process_msgs(const uint32_t t_ms)
        {
            Stat stat = false;
            message_packet pkt;
            if (queue_.front(pkt, t_ms))
            {
                stat = true;
                process_pkt(pkt);
                while (queue_.front(pkt))
                {
                    process_pkt(pkt);
                }
            }
            return stat;
        }

    private:
        using message_packet = etl::message_packet<TMsgs...>;
        BlockingMsgQueue<iPkt*, QueueDepth> queue_;

        inline void process_pkt(message_packet& pkt)
        {
            etl::imessage &msg = pkt.get();
            Base_t::receive(msg);
        }
    };
}
