
#pragma once

#include <etfw/svcs/App.hpp>
#include <etfw/msg/Message.hpp>
#include <etfw/msg/MsgBroker.hpp>
#include <etfw/msg/Router.hpp>
#include <etfw/msg/BlockingMsgQueue.hpp>
#include "StatsMsgTbl.hpp"

namespace stats_mon
{

    template <typename Handler>
    class idynamic_pipe : public etl::imessage_router
    {
    public:
        using Base_t = etl::imessage_router;
        using PipeId_t = etl::message_router_id_t;
        using Subscription_t = etfw::msg::Subscription;

        idynamic_pipe(PipeId_t id, Handler& handler):
            Base_t(id),
            handler_(handler),
            msg_subscription_(*this)
        {}

        virtual void receive(const etl::imessage& msg) override
        {
            handler_.handle(msg);
        }

        virtual bool accepts(etl::message_id_t id) const override
        {
            if (msg_subscription_.is_subscribed(id))
            {
                return true;
            }
            return false;
        }

        virtual bool is_null_router() const override
        {
            return false;
        }

        virtual bool is_producer() const override
        {
            return false;
        }

        virtual bool is_consumer() const override
        {
            return true;
        }

        inline bool subscribe(const etfw::msg::MsgId_t id)
        {
            return msg_subscription_.subscribe(id);
        }

        inline bool unsubscribe(const etfw::msg::MsgId_t id)
        {
            return msg_subscription_.unsubscribe(id);
        }

        inline Subscription_t& subscription() { return msg_subscription_; }

    private:
        Handler& handler_;
        etfw::msg::Subscription msg_subscription_;
    };

    template <typename THandler, size_t QDepth>
    class queued_dynamic_pipe : public idynamic_pipe<THandler>
    {
    public:
        using Base_t = idynamic_pipe<THandler>;
        using PipeId_t = typename Base_t::PipeId_t;
        using Base_t::accepts;

        using Status = bool;

        queued_dynamic_pipe(PipeId_t id, THandler& handler):
            Base_t(id, handler)
        {}

        void receive(const etl::imessage& msg) override
        {
            if (accepts(msg.get_message_id()))
            {
                packet pkt(msg.get_message_id());
                queue.emplace(pkt);
            }
        }

        Status receive_msgs(const uint32_t time_ms)
        {
            Status stat = false;
            packet pkt;
            if (queue.front(pkt, time_ms))
            {
                stat = true;
                process_pkt(pkt);
                while (queue.front(pkt))
                {
                    process_pkt(pkt);
                }
            }
            return stat;
        }

    private:
        class packet : public etl::imessage
        {
        public:
            packet(MsgId_t id):
                etl::imessage(id)
            {}

            packet():
                etl::imessage(0)
            {}
        };
        etfw::msg::BlockingMsgQueue<packet, 20> queue;

        inline void process_pkt(packet& pkt)
        {
            Base_t::receive(pkt);
        }
    };

    class StatsHandler : public MsgTblObserver
    {
    public:

        using StatsPipe_t = queued_dynamic_pipe<StatsHandler, 20>;

        /// @brief Construct from pre-existing message table
        /// @param fw_proxy Framework access proxy
        /// @param msg_tbl Default message table
        StatsHandler(
            etfw::iApp::AppFwProxy& fw_proxy
        ):
            fw_proxy_(fw_proxy),
            stats_pipe_(AppId, *this)
        {}

        void process_stats_messages()
        {
            // receive_msgs will dispatch msgs to the handler
            // method. No need for timeout. Assuming called during
            // wakeup
            if (stats_pipe_.receive_msgs(0))
            {
                fw_proxy_.log(etfw::LogLevel::DEBUG,
                    "Received stats message");
            }
            else
            {
                //fw_proxy_.log(etfw::LogLevel::DEBUG,
                //    "Did not receive stats message");
            }
        }

        void handle(const etl::imessage& msg)
        {
            fw_proxy_.log(etfw::LogLevel::DEBUG,
                "Received message ID 0x%X",
                msg.get_message_id());
            // In a real application, the message contents might
            // be forwarded to a peripheral or logged
        }

        void notification(EntryAdded evt)
        {
            auto stat = stats_pipe_.subscribe(evt.Id);
            if (stat)
            {
                printf("Subscribing to %d\n\n", evt.Id);
                fw_proxy_.subscribe(stats_pipe_.subscription());
                fw_proxy_.log(etfw::LogLevel::INFO,
                    "Successfully subscribed to MSD ID %d",
                    evt.Id);
            }
            else
            {
                fw_proxy_.log(etfw::LogLevel::ERROR,
                    "Failed to subscribe to msg ID 0x%X",
                    evt.Id);
            }
        }

        void notification(EntryRemoved evt)
        {
            auto stat = stats_pipe_.unsubscribe(evt.Id);
            if (stat)
            {
                fw_proxy_.log(etfw::LogLevel::INFO,
                    "Successfully unsubscribed to MSD ID 0x%X",
                    evt.Id);
            }
            else
            {
                fw_proxy_.log(etfw::LogLevel::ERROR,
                    "Failed to unsubscribe from msg ID 0x%X",
                    evt.Id);
            }
        }

    private:
        etfw::iApp::AppFwProxy& fw_proxy_;
        StatsPipe_t stats_pipe_;
    };
}
