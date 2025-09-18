
#pragma once

#include "MsgBroker.hpp"
#include "BlockingMsgQueue.hpp"
#include <etl/message_router.h>

namespace etfw::msg
{
    class iPipe : public etl::imessage_router
    {
    public:
        using Base_t = etl::imessage_router;
        using Subscription_t = Subscription;
        using PipeId_t = etl::message_router_id_t;
        static constexpr PipeId_t DefaultPipeId = 0;

        bool is_null_router() const override { return false; }
        bool is_producer() const override { return false; }
        bool is_consumer() const override { return true; }

        bool accepts(etl::message_id_t id) const override
        {
            return subbed_msgs_.has(id);
        }

        inline Subscription_t& subscription() { return subbed_msgs_; }
        inline const Subscription_t& subscription() const { return subbed_msgs_; }

    protected:
        iPipe():
            Base_t(DefaultPipeId),
            subbed_msgs_(*this)
        {}

        iPipe(PipeId_t id):
            Base_t(id),
            subbed_msgs_(*this)
        {}

    private:
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
        //BlockingMsgQueue
    };

    template <typename THandler, typename... TMsgs>
    class StaticPipe : public iPipe
    {
    public:
        using Base_t = iPipe;
    };
}
