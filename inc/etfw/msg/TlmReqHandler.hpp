
#pragma once

#include "Message.hpp"
#include "Router.hpp"
#include <etl/tuple.h>

namespace etfw::msg
{
    template <SvcId_t AppIdV, typename... TMsgs>
    class TlmStorage
    {
    public:
        TlmStorage() = default;
        TlmStorage(const TlmStorage&) = default;
        TlmStorage(TlmStorage&&) = default;

        template <typename... Args>
        TlmStorage(Args&&... args):
            msgs_(etl::forward<Args>(args)...)
        {}

        /// @brief Get a message object
        /// @tparam MsgT Message type to get
        /// @return The message instance
        template <typename MsgT>
        inline MsgT& get()
            { return etl::get<MsgT>(msgs_); }

        /// @brief Get a const message object
        /// @tparam MsgT Message type
        /// @return Const message instance
        template <typename MsgT>
        inline const MsgT& get() const
            { return etl::get<MsgT>(msgs_); }

        template <MsgId_t RequestId, size_t Idx = 0>
        decltype(auto) find_by_req_id()
        {
            const MsgId_t tlm_id = ConvertToTlmId<RequestId>::value;

            if constexpr (Idx >= etl::tuple_size_v<MsgContainer_t>)
            {
                return static_cast<void*>(nullptr);
            }
            else
            {
                using Element_t = etl::tuple_element_t<Idx, MsgContainer_t>;
                if constexpr (Element_t::ID == tlm_id)
                {
                    return &etl::get<Idx>(msgs_);
                }
                else
                {
                    return find_by_req_id<tlm_id, Idx+1>();
                }
            }
        }

    private:
        /// @brief Storage type for message instances
        using MsgContainer_t = etl::tuple<TMsgs...>;

        /// @brief Message objects
        MsgContainer_t msgs_;
    };


    template <SvcId_t AppIdV, typename... TMsgs>
    class TlmReqHandler
    {
    public:
        /// @brief Msg storage type
        using MsgStorage_t = TlmStorage<AppIdV, TMsgs...>;

        /// @brief Telemetry request alias
        /// @tparam FuncIdV App's telemetry ID
        template <FuncId_t FuncIdV>
        using tlm_req_msg = telemetry_request<AppIdV, FuncIdV>;

        /// @brief Alias to return a tlm message's function ID
        /// @tparam TTlmMsg Telemetry message type
        template <typename TTlmMsg>
        using get_func_id = ExtractFuncId<TTlmMsg>;

        /// @brief Default telemetry -> request converter
        /// @tparam TStatsMsg Telemetry message type
        template <typename TStatsMsg>
        struct MakeReqMsg
        {
            using type = tlm_req_msg<get_func_id<TStatsMsg>::value>;
        };

        TlmReqHandler():
            tlm_req_pipe_(*this, 2)
        {}

        inline MsgStorage_t& messages() { return tlm_msgs_; }

        /// @brief Send tlm request handler. Will find and send the target
        ///     tlm message.
        /// @tparam Msg TLM REQ message type
        /// @param msg Unused req message
        template <typename Msg>
        void receive(const Msg& msg)
        {
            auto* msg_to_send = tlm_msgs_.template find_by_req_id<Msg::ID>();
            if (msg_to_send)
            {
                printf("Found msg 0x%X\n",
                    msg.get_message_id());
            }
        }

        const char* name_raw() { return "TMP"; }

    private:
        /// @brief Alias for this class type
        using This_t = TlmReqHandler<AppIdV, TMsgs...>;

        template <typename... ReqMsgs>
        struct BuildTlmReqPipe
        {
            using type = Router<This_t, 1,
                typename MakeReqMsg<ReqMsgs>::type...>;
        };

        /// @brief Pipe for processing send message requests
        using StatsReqPipe_t = typename BuildTlmReqPipe<TMsgs...>::type;
        StatsReqPipe_t tlm_req_pipe_;

        /// @brief Message instances
        MsgStorage_t tlm_msgs_;
    };
}
