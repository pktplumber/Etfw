
#pragma once

#include <etl/message_router.h>
#include "MsgBroker.hpp"
#include "BlockingMsgQueue.hpp"

namespace etfw {
namespace Msg {

    template <typename... TMsgs>
    constexpr auto to_msg_id_list() {
        return MsgIdContainer{ TMsgs::ID... };
    }

    /**
     * @brief Generic message handler class.
     * 
     * @tparam THandler Service/component handler type.
     * @tparam TMsgLimit Number of messages allowed before processing. Not used for base.
     * @tparam TMsgs Message structs.
     */
    template <typename THandler, size_t TMsgLimit, typename... TMsgs>
    class MsgRtr : public etl::message_router<
        MsgRtr<THandler, TMsgLimit, TMsgs...>,
        TMsgs...>
    {
        public:
            using Base_t = etl::message_router<
                MsgRtr<THandler, TMsgLimit, TMsgs...>,
                TMsgs...>;

            MsgRtr(THandler& component):
                Base_t(static_cast<etl::message_id_t>(component.ID)),
                Handler_(component),
                IdList(to_msg_id_list<TMsgs...>()),
                SubbedMsgs(*this, IdList) {}
            
            template <typename Msg>
            void on_receive(const Msg& msg)
            {
                Handler_.receive(msg);
            }

            void on_receive_unknown(const etl::imessage& msg)
            {
                printf("[%s]: Got unkown message\n",
                    Handler_.name_raw());
            }

            inline Subscription& subscription(void)
            {
                return SubbedMsgs;
            }
        
        protected:
            THandler& Handler_;
            MsgIdContainer IdList;
            Subscription SubbedMsgs;
    };

    /**
     * @brief Queue-based message router/handler
     * 
     * @tparam THandler Service/component type
     * @tparam TMsgLimit Message limit/queue depth
     * @tparam TMsgs Message types
     */
    template <typename THandler, size_t TMsgLimit, typename... TMsgs>
    class QueuedMsgRtr : public MsgRtr<THandler, TMsgLimit, TMsgs...>
    {
        public:
            using Base_t = MsgRtr<THandler, TMsgLimit, TMsgs...>;
            using Base_t::accepts;

            QueuedMsgRtr(THandler& component):
                Base_t(component),
                Enabled(true) {}

            void receive(const etl::imessage& msg) override
            {
                if (accepts(msg))
                {
                    queue.emplace(msg);
                    // TODO: send events
                }
            }

            void process_msg_queue(const uint32_t time_ms)
            {
                message_packet pkt;
                if (queue.front(pkt, time_ms))
                {
                    process_pkt(pkt);
                    while (queue.front(pkt))
                    {
                        process_pkt(pkt);
                    }
                }
            }

            inline void enable(void) { Enabled = true; }

            inline void disable(void) { Enabled = false; }

            inline bool is_enabled(void) const { return Enabled; }
        
        private:
            using message_packet = etl::message_packet<TMsgs...>;
            BlockingMsgQueue<message_packet, TMsgLimit> queue;
            volatile bool Enabled;

            inline void process_pkt(message_packet& pkt)
            {
                etl::imessage &msg = pkt.get();
                Base_t::receive(msg);
            }
    };

}
}
