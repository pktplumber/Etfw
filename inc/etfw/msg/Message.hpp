
#pragma once

#include <cstdint>
#include <etl/message.h>
#include "svcs/SvcTypes.hpp"

#ifndef ETFW_MSG_FUNC_ID_OFFSET
#define ETFW_MSG_FUNC_ID_OFFSET     0
#endif

#ifndef ETFW_MSG_TYPE_ID_OFFSET
#define ETFW_MSG_TYPE_ID_OFFSET     (ETFW_MSG_FUNC_ID_OFFSET + 8)
#endif

#ifndef ETFW_MSG_MOD_ID_OFFSET
#define ETFW_MSG_MOD_ID_OFFSET      (ETFW_MSG_TYPE_ID_OFFSET + 8)
#endif

namespace etfw::msg
{

    // ~~~~~~~~~~~~~~~~~ Message Constants ~~~~~~~~~~~~~~~~~

    /// @brief Bit-offset of the module ID within a message ID
    static constexpr size_t ModIdOffset = ETFW_MSG_MOD_ID_OFFSET;

    /// @brief Bit-offset of the message ID within a message ID
    static constexpr size_t TypeIdOffset = ETFW_MSG_TYPE_ID_OFFSET;

    /// @brief Bit-offset of the function ID within a message ID
    static constexpr size_t FuncIdOffset = ETFW_MSG_FUNC_ID_OFFSET;


    // ~~~~~~~~~~~~~~~~~ Type/Id Definitions ~~~~~~~~~~~~~~~~~

    /// @brief Specifies the owning application of a message.
    using MsgModuleId_t = AppId_t;

    /// @brief Reserved message module ID.
    constexpr MsgModuleId_t MsgModuleIdRsvd = 0;

    /// @brief Indicates the function/dataset of a message.
    using FuncId_t = uint8_t;

    /// @brief Used to route messages to an individual node.
    using MsgId_t = uint32_t;

    /// @brief Message type IDs
    enum MsgType_t : uint8_t
    {
        WAKEUP = 0, //< Used to wakeup scheduled services.
        CMD = 1,    //< Request/command an service operation
        TLM = 2,    //< Status message type
        RESP = 3,   //< Command/request status response
        TLM_REQ = 4, //< Request to send a telemetry message
        COUNT
    };

    // ~~~~~~~~~~~~~~~~~ Type traits ~~~~~~~~~~~~~~~~~
    template <typename T>
    struct ExtractModId
    {
        static constexpr MsgModuleId_t value = T::ID>>ModIdOffset;
    };

    template <typename T>
    struct ExtractFuncId
    {
        static constexpr FuncId_t value = T::ID & 0xFF;
    };

    template <typename T>
    struct ExtractMsgType
    {
        static constexpr MsgType_t value = (T::ID >>TypeIdOffset)&0xFF;
    };

    template <MsgId_t IdV>
    struct ConvertToTlmId
    {
        static constexpr MsgId_t value = ((IdV & 0xFFFF00FF) | 
            (MsgType_t::TLM << TypeIdOffset));
    };

    // ~~~~~~~~~~~~~~~~~ Helper methods ~~~~~~~~~~~~~~~~~

    /// @brief Extracts the MsgModuleId_t from a MsgId_t.
    /// @param[in] msg_id Message ID.
    /// @return constexpr MsgModuleId_t The message ID's module.
    constexpr MsgModuleId_t to_mod_id(MsgId_t msg_id)
    {
        return static_cast<MsgModuleId_t>(msg_id>>ModIdOffset);
    }

    constexpr MsgType_t to_msg_type_id(const MsgId_t msg_id)
    {
        return static_cast<MsgType_t>((msg_id>>TypeIdOffset) & 0xFF);
    }

    /// @brief Extracts the MsgModuleId_t from a MsgId_t.
    /// @param[in] msg_id Message ID.
    /// @return constexpr FuncId_t The message's function ID.
    constexpr FuncId_t to_func_id(MsgId_t msg_id)
    {
        return static_cast<FuncId_t>(msg_id & 0xFF);
    }

    /// @brief Compile time method for creating a message ID. 
    /// @tparam TModId Module ID.
    /// @tparam TCmdId Function ID.
    /// @return constexpr auto MsgId_t
    template <MsgModuleId_t VModId, MsgType_t VMsgType, FuncId_t VFuncId>
    constexpr auto to_msg_id()
    {
        return static_cast<MsgId_t>(
            (VModId << ModIdOffset) |
            (VMsgType << TypeIdOffset) |
            (VFuncId));
    }


    // ~~~~~~~~~~~~~~~~~ Message Class Types ~~~~~~~~~~~~~~~~~

    struct iBaseMsg : public etl::imessage
    {
        iBaseMsg(MsgId_t id):
            etl::imessage(id)
        {}

        iBaseMsg():
            etl::imessage(0)
        {}
    };

    struct iMsg : public etl::imessage
    {
        size_t Size;

        iMsg(MsgId_t id, size_t sz):
            etl::imessage(id),
            Size(sz)
        {}
    };

    /// @brief ETFW message base
    /// @tparam TModId ID of the message's owner module
    /// @tparam TFuncId Message function ID
    template <MsgModuleId_t TModId, MsgType_t VMsgType, FuncId_t TFuncId>
    struct BaseMsg : public etl::message<
        to_msg_id<TModId, VMsgType, TFuncId>()>
    {};

    /// @brief Wakeup message type
    /// @tparam TModId Module/user of wakeup message
    template <MsgModuleId_t TModId>
    struct wakeup : public BaseMsg<TModId, WAKEUP, 0>
    {};

    /// @brief Command message base type
    /// @tparam ModIdV ID of the "owner"/receiving module
    /// @tparam FuncIdV Command function identifier
    template <MsgModuleId_t ModIdV, FuncId_t FuncIdV>
    struct command : public BaseMsg<
        ModIdV, MsgType_t::CMD, FuncIdV>
    {
        MsgModuleId_t Source;
        bool ResponseExpected;

        /// @brief Default constructor. Source unused.
        command():
            Source(MsgModuleIdRsvd),
            ResponseExpected(false)
        {}

        /// @brief Construct with source information
        /// @param sender Source/sender of command
        command(
            MsgModuleId_t sender
        ):
            Source(sender),
            ResponseExpected(false)
        {}

        /// @brief Command with response expected
        /// @param sender Source/sender of command
        /// @param rx_resp Flag indicating a sender expects a response
        command(
            MsgModuleId_t sender,
            bool rx_resp
        ):
            Source(sender),
            ResponseExpected(rx_resp)
        {}
    };

    /// @brief Base for status/telemetry messages
    /// @tparam ModIdV ID of "owner"/sending module
    /// @tparam FuncIdV Module's status message ID
    template <MsgModuleId_t ModIdV, FuncId_t FuncIdV>
    struct telemetry : public BaseMsg<
        ModIdV, MsgType_t::TLM, FuncIdV>
    {};

    template <MsgModuleId_t ModIdV, FuncId_t TlmFuncIdV>
    struct telemetry_request : public BaseMsg<
        ModIdV, MsgType_t::TLM_REQ, TlmFuncIdV>
    {};

    /// @brief Function/command codes common to all apps/modules.
    enum class CommonCmdCodes : FuncId_t
    {
        NOOP = 0,
    };

    /**
     * @brief Generic no-op command message.
     * 
     * @tparam TModId Destination module ID.
     */
    template <MsgModuleId_t TModId>
    struct NoopCmd : etl::message<to_msg_id<TModId, 
        static_cast<uint8_t>(CommonCmdCodes::NOOP)>()> {};
    

}
