
#pragma once

#include "Message.hpp"
#include <etl/message_broker.h>

namespace etfw::msg
{
    /// @brief Message ID subscription class
    class subscription : public etl::message_broker::subscription
    {
    public:
        // Base class type
        using Base_t = etl::message_broker::subscription;

        /// @brief Alias for message id span/view type
        using MsgSpan_t = etl::message_broker::message_id_span_t;

        /// @brief List of message IDs. TODO: needs to be static type
        using IdContainer_t = std::vector<MsgId_t>;

        using Status = bool;

        /// @brief Construct a msg subscription with an initializer list
        /// @param module Router/pipe/module subscribed to the msg ids
        /// @param ids Message IDs the router is subscribed to
        subscription(
            etl::imessage_router& module,
            std::initializer_list<MsgId_t> ids
        ):
            Base_t(module),
            ids_(ids)
        {}

        /// @brief Construct a subscription from a message ID list
        /// @param module The router owning the subscription
        /// @param msg_ids Message IDs the router is subscribed to
        subscription(
            etl::imessage_router& module,
            IdContainer_t &msg_ids
        ):
            Base_t(module),
            ids_(msg_ids)
        {}

        /// @brief Construct empty subscription object
        /// @param pipe The pipe/router owning the subscription
        subscription(
            etl::imessage_router& pipe
        ):
            Base_t(pipe)
        {}

        /// @brief Returns a view of the subscribed message IDs
        /// @return Message ID view
        MsgSpan_t message_id_list() const override
        {
            return MsgSpan_t(ids_.begin(), ids_.end());
        }

        /// @brief Add an ID to the subscription
        /// @param id Message ID to subscribe to
        /// @return 
        Status subscribe(const MsgId_t id)
        {
            ids_.push_back(id);
            return true;
        }

        /// @brief Remove an ID to the subscription
        /// @param id Message ID to unsubscribe from
        /// @return 
        Status unsubscribe(const MsgId_t id)
        {
            ids_.erase(std::remove(
                ids_.begin(), ids_.end(), id),
                ids_.end());
            return true;
        }

        /// @brief Check if a message id is in this subscription
        /// @param id Message id to check for
        /// @return True if subscribed, otherwise false
        bool is_subscribed(const MsgId_t id) const
        {
            for (auto& id_: ids_)
            {
                if (id_ == id)
                {
                    return true;
                }
            }
            return false;
        }

        /// @brief Check if a message id is in this subscription
        /// @param id Message id to check for
        /// @return True if subscribed, otherwise false
        inline bool has(const MsgId_t id) const
        {
            return is_subscribed(id);
        }

    private:
        IdContainer_t ids_;
    };
}
