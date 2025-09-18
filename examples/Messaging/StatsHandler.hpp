
#pragma once

#include "Common.hpp"
#include <etfw/msg/Router.hpp>
#include <etfw/msg/Message.hpp>
#include <etl/tuple.h>

template <etfw::msg::FuncId_t TargetID, typename TupleT, size_t Idx = 0>
decltype(auto) find_by_id(TupleT& tuple)
{
    if constexpr (Idx >= etl::tuple_size_v<TupleT>)
    {
        return nullptr;
    }
    else
    {
        using Element_t = etl::tuple_element_t<Idx, TupleT>;
        if constexpr (Element_t::ID == TargetID)
        {
            return &etl::get<Idx>(tuple);
        }
        else
        {
            return find_by_id<TargetID, TupleT, Idx+1>(tuple);
        }
    }
}

template <etfw::SvcId_t AppIdV, typename... TMsgs>
class TlmStorage
{
public:
    TlmStorage() = default;
    TlmStorage(const TlmStorage&) = default;
    TlmStorage(TlmStorage&&) = default;
    //TlmStorage& operator=(const TlmStorage&) = default;
    //TlmStorage& operator=(const TlmStorage&&) = default;

    template <typename... Args>
    TlmStorage(Args&&... args):
        msgs_(etl::forward<Args>(args)...)
    {}

    /// @brief Get a message object
    /// @tparam MsgT Message type to get
    /// @return The message instance
    template <typename MsgT>
    inline MsgT& get() { return etl::get<MsgT>(msgs_); }

    /// @brief Get a const message object
    /// @tparam MsgT Message type
    /// @return Const message instance
    template <typename MsgT>
    inline const MsgT& get() const { return etl::get<MsgT>(msgs_); }

    template <typename MsgT>
    inline MsgT& find_by_id(const etfw::msg::FuncId_t id)
    {
        
    }

private:
    /// @brief Storage type for message instances
    using MsgContainer_t = etl::tuple<TMsgs...>;

    /// @brief Message objects
    MsgContainer_t msgs_;
};

/// @brief Stats/telemetry message handler
/// @tparam ...TMsgs Stats message types
/// @tparam AppIdV Application ID
template <etfw::SvcId_t AppIdV, typename... TMsgs>
class StatsManager
{
public:

    using MsgStorage_t = TlmStorage<AppIdV, TMsgs...>;

    /// @brief Telemetry request alias
    /// @tparam FuncIdV App's telemetry ID
    template <etfw::msg::FuncId_t FuncIdV>
    using tlm_req_msg = etfw::msg::telemetry_request<AppIdV, FuncIdV>;

    /// @brief Alias to return a tlm message's function ID
    /// @tparam TTlmMsg Telemetry message type
    template <typename TTlmMsg>
    using get_func_id = etfw::msg::ExtractFuncId<TTlmMsg>;

    /// @brief Default telemetry -> request converter
    /// @tparam TStatsMsg Telemetry message type
    template <typename TStatsMsg>
    struct MakeReqMsg
    {
        using type = tlm_req_msg<get_func_id<TStatsMsg>::value>;
    };

    StatsManager():
        tlm_req_pipe_(*this, 2)
    {}

    inline MsgStorage_t& messages() { return tlm_msgs_; }

    template <typename Msg>
    void receive(const Msg& msg)
    {
        //const etfw::msg::FuncId_t id = etfw::msg::to_func_id(Msg::ID);
        //auto* msg_to_send = find_by_id<id, >(tlm_msgs_);
        //if (msg_to_send)
        //{
        //    printf("Found msg 0x%X\n",
        //        msg.get_message_id());
        //}
        //else
        //{
        //    printf("Failed to find msg 0x%X\n",
        //        msg.get_message_id());
        //}
    }

    const char* name_raw() { return "TMP"; }

private:
    /// @brief Alias for this class type
    using This_t = StatsManager<AppIdV, TMsgs...>;

    template <typename... ReqMsgs>
    struct BuildTlmReqPipe
    {
        using type = etfw::msg::Router<This_t, 1,
            typename MakeReqMsg<ReqMsgs>::type...>;
    };

    /// @brief Pipe for processing send message requests
    using StatsReqPipe_t = typename BuildTlmReqPipe<TMsgs...>::type;
    StatsReqPipe_t tlm_req_pipe_;

    /// @brief Message instances
    MsgStorage_t tlm_msgs_;
};
