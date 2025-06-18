
#pragma once

#include <etl/message_broker.h>

#ifndef MSG_MAX_NUM_SUBSCRIPTIONS
#define MSG_MAX_NUM_SUBSCRIPTIONS   64
#endif

namespace etfw {
namespace Msg {

using MsgId = etl::message_id_t;

using MsgIdContainer = std::vector<MsgId>;

class Subscription : public etl::message_broker::subscription
{
    public:
        using MsgSpan_t = etl::message_broker::message_id_span_t;

        Subscription(etl::imessage_router& module,
            std::initializer_list<MsgId> ids)
            : etl::message_broker::subscription(module)
            , IdList(ids)
        {}
        
        Subscription(etl::imessage_router& module, MsgIdContainer &msg_ids)
            : etl::message_broker::subscription(module)
            , IdList(msg_ids)
        {}

        MsgSpan_t message_id_list() const override
        {
            return MsgSpan_t(IdList.begin(), IdList.end());
        }
    
    private:
        /// TODO: replace with static container [MSG_MAX_NUM_SUBSCRIPTIONS]
        std::vector<MsgId> IdList;
};

using Broker = etl::message_broker;

}
}
