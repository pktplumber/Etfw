
#pragma once

#include <etl/message_broker.h>

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
    using Broker = etl::message_broker;



    class iSubscription
    {
    public:
        using Base_t = etl::message_broker::subscription;
        using MsgSpan_t = etl::message_broker::message_id_span_t;


    };

}
