
#pragma once

#include "Subscription.hpp"
#include "BlockingMsgQueue.hpp"
#include "Pkt.hpp"
#include <etl/message_router.h>
#include <etl/initializer_list.h>

namespace etfw::msg
{
    // Forward declaration for queued pipe
    class SharedMessage;

    /// @brief Message pipe interface. Router with message subscription
    class iPipe : public etl::imessage_router
    {
    public:
        using Base_t = etl::imessage_router;
        using Subscription_t = subscription;

        /// @brief Pipe priority type. 0 = highest, MaxPriority = lowest
        using Priority_t = etl::message_router_id_t;

        /// @brief Maximum pipe priority
        static constexpr Priority_t MaxPriority =
            etl::imessage_router::MAX_MESSAGE_ROUTER;

        /// @brief Default lowest priority
        static constexpr Priority_t DefaultPriority = MaxPriority;

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

        /// @brief Subscribe this pipe to a message ID
        /// @param id ID to subscribe to.
        /// @return Subscription status
        inline Subscription_t::Status subscribe(const MsgId_t id)
        {
            return subbed_msgs_.subscribe(id);
        }

        /// @brief Unsubscribe this pipe from a message ID
        /// @param id ID to unsubscribe from.
        /// @return Subscription status
        inline Subscription_t::Status unsubscribe(const MsgId_t id)
        {
            return subbed_msgs_.unsubscribe(id);
        }

    protected:
        /// @brief Default constructor. Builds empty message subscription
        iPipe():
            Base_t(DefaultPriority),
            subbed_msgs_(*this)
        {}

        /// @brief Construct pipe with custom ID an empty msg subscription
        /// @param id Pipe ID
        iPipe(Priority_t prior):
            Base_t(prior),
            subbed_msgs_(*this)
        {}

        /// @brief Construct pipe with custom ID and known msg subscription
        /// @param id Pipe ID
        /// @param msg_ids Message IDs to subscribe to
        iPipe(
            Priority_t prior,
            std::initializer_list<MsgId_t> msg_ids
        ):
            Base_t(prior),
            subbed_msgs_(*this, msg_ids)
        {}

        iPipe(
            std::initializer_list<MsgId_t> msg_ids
        ):
            Base_t(DefaultPriority),
            subbed_msgs_(*this, msg_ids)
        {}

        template <typename... TMsgs>
        iPipe():
            Base_t(DefaultPriority),
            subbed_msgs_(*this, TMsgs{}...)
        {}

        template <typename... TMsgs>
        iPipe(Priority_t prior):
            Base_t(prior),
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

        Pipe(Priority_t prior, THandler& handler):
            Base_t(prior),
            handler_(handler)
        {}

        Pipe(
            Priority_t prior,
            THandler& handler,
            std::initializer_list<MsgId_t> msg_ids
        ):
            Base_t(prior, msg_ids),
            handler_(handler)
        {}

        Pipe(
            THandler& handler,
            std::initializer_list<MsgId_t> msg_ids
        ):
            Base_t(msg_ids),
            handler_(handler)
        {}

        /// @brief Handles a message sent to this pipe. 
        /// @details Overrides etl::imessage_router (base of iPipe) 
        ///          "receive" pure virtual method.
        /// @param[in] msg Msg to handle
        virtual void receive(const etl::imessage& msg) override
        {
            //handler_.handle(msg);
            handler_.handle(convert<iBaseMsg>(msg));
        }

    protected:
        inline THandler& get_handler() { return handler_; }

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
        using Base_t = Pipe<THandler>;
        using Priority_t = typename Base_t::Priority_t;
        using Base_t::accepts;
        using QueueMsg_t = etl::shared_message;
        using DropCounter_t = uint32_t;

        QueuedPipe(
            THandler& handler
        ):
            Base_t(handler),
            drops_(0)
        {}

        QueuedPipe(
            Priority_t prior,
            THandler& handler
        ):
            Base_t(prior, handler),
            drops_(0)
        {}

        QueuedPipe(
            Priority_t prior,
            THandler& handler,
            std::initializer_list<MsgId_t> msg_ids
        ):
            Base_t(prior, handler, msg_ids),
            drops_(0)
        {}

        QueuedPipe(
            THandler& handler,
            std::initializer_list<MsgId_t> msg_ids
        ):
            Base_t(handler, msg_ids),
            drops_(0)
        {}

        virtual void receive(etl::shared_message msg) override
        {
            if (!queue_.full())
            {
                queue_.push(msg);
            }
            else
            {
                drops_++;
            }
        }

        void process_queue(const uint32_t t_ms)
        {
            while (!queue_.empty())
            {
                etl::shared_message sm = queue_.front(t_ms);
                Base_t::receive(sm.get_message());
                queue_.pop();
            }
        }

        /// @brief Get number of drops due to pipe being full
        /// @details The drop counter can be used to profile the performance
        ///     of a service. For example, if this pipe is used for wakeups,
        ///     the drop count will increase if the scheduled process takes
        ///     longer then the wakeup period.
        /// @return Number of dropped messages
        inline DropCounter_t drops() const { return drops_; }

        /// @brief Reset the drop counter to 0
        inline void reset_drops() { drops_ = 0; }

    private:
        BlockingMsgQueue<QueueMsg_t, QueueDepth> queue_;
        DropCounter_t drops_;
    };

    /// @brief Synchronous pipe with static message subscription.
    ///        Message subscription is determined at compile time.
    template <typename THandler, typename... TMsgs>
    class StaticPipe : public Pipe<THandler>
    {
    public:
        using Base_t = Pipe<THandler>;
        using Priority_t = typename Base_t::Priority_t;
        using Base_t::receive;

        StaticPipe(THandler& handler):
            Base_t(handler, {TMsgs::ID...})
        {}

        StaticPipe(
            Priority_t prior,
            THandler& handler
        ):
            Base_t(prior, handler, {TMsgs::ID...})
        {}

        void receive(const etl::imessage& msg) override
        {
            const bool was_handled = (receive_msg_type<TMsgs>(msg) || ...);
            if (!was_handled)
            {
                get_handler().handle_unknown_msg(convert<iBaseMsg>(msg));
            }
        }

    private:
        using Base_t::get_handler;
        using This_t = StaticPipe<THandler, TMsgs...>;

        template <typename TMsg>
        bool receive_msg_type(const etl::imessage& msg)
        {
            if (TMsg::ID == msg.get_message_id())
            {
                get_handler().handle(static_cast<const TMsg&>(msg));
                return true;
            }
            return false;
        }
    };

    /// @brief Queued pipe with static message subscription. Message
    ///        subscription is determined at compile time
    /// @details The StaticQueuedPipe is used for receiving asynchronous messages
    ///          copied directly into the pipe (i.e. no zero copy). This should
    ///          be used for handling commands or status messages that will not
    ///          change throughout the lifetime of the program that do not
    ///          require high performance.
    /*template <typename THandler, size_t QueueDepth, typename... TMsgs>
    class StaticQueuedPipe : public QueuedPipe<THandler, QueueDepth>
    {
    public:
        using Base_t = QueuedPipe<THandler, QueueDepth>;
        using Priority_t = typename Base_t::Priority_t;
        using Base_t::accepts;
        using Base_t::QueueMsg_t;

        StaticQueuedPipe(THandler& handler):
            Base_t(handler)
        {}

        StaticQueuedPipe(Priority_t prior, THandler& handler):
            Base_t(prior, handler)
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
        BlockingMsgQueue<QueueMsg_t, QueueDepth> queue_;

        inline void process_pkt(message_packet& pkt)
        {
            etl::imessage &msg = pkt.get();
            Base_t::receive(msg);
        }
    };*/

    template <typename THandler, size_t Depth, typename... TMsgs>
    class StaticQueuedPipe : public QueuedPipe<THandler, Depth>
    {
    public:
        using Base_t = QueuedPipe<THandler, Depth>;
    protected:
    private:
    };

    /// @brief CRTP-based wakeup pipe for asynchronous services
    /// @tparam THandler Owner application type
    /// @tparam AppId Application ID
    template <typename THandler, MsgModuleId_t AppId>
    class QueuedWakeupPipe : public QueuedPipe<
        QueuedWakeupPipe<THandler, AppId>, 1>
    {
    public:
        using Base_t = QueuedPipe<
            QueuedWakeupPipe<THandler, AppId>, 1>;
        using WakeupMsg_t = wakeup_msg<AppId>;
        using Priority_t = typename Base_t::Priority_t;
        using Base_t::process_queue;
        using Status = bool;

        QueuedWakeupPipe(THandler& handler):
            Base_t(*this, {WakeupMsg_t::ID}),
            handler_(handler)
        {}

        QueuedWakeupPipe(THandler& handler, Priority_t prior):
            Base_t(prior, *this, {WakeupMsg_t::ID}),
            handler_(handler)
        {}

        Status wait(const uint32_t timeout_ms)
        {
            process_queue(timeout_ms);
            return true;
        }

        void handle(const iBaseMsg& wkup)
        {
            handler_.on_wakeup();
        }

    private:
        THandler& handler_;
    };
}
