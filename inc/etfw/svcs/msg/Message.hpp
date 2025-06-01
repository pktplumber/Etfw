
#pragma once

#include <cstdint>
#include <etl/message.h>
#include "../AppTypes.hpp"

namespace etfw {
    namespace Msg {

        /** @brief Specifies the owning application of a message. */
        using MsgModuleId_t = AppId_t;

        /** @brief Reserved message module ID. */
        constexpr MsgModuleId_t MsgModuleIdRsvd = 0;

        /** @brief Indicates the function/dataset of a message. */
        using MsgFuncId_t = uint8_t;

        /** @brief Used to route messages to an individual node. */
        using MsgId_t = etl::message_id_t;

        enum MsgType : uint8_t
        {
            WAKEUP = 0,
            CMD = 1,
            TLM = 2,
            RESP = 3,
            COUNT
        };

        /**
         * @brief Extracts the MsgModuleId_t from a MsgId_t.
         * 
         * @param[in] msg_id Message ID.
         * @return constexpr MsgModuleId_t The message ID's module.
         */
        constexpr MsgModuleId_t to_mod_id(MsgId_t msg_id)
        {
            return static_cast<MsgModuleId_t>(msg_id>>8);
        }

        /**
         * @brief Extracts the MsgModuleId_t from a MsgId_t.
         * 
         * @param[in] msg_id Message ID.
         * @return constexpr MsgFuncId_t The message's function ID.
         */
        constexpr MsgFuncId_t to_func_id(MsgId_t msg_id)
        {
            return static_cast<MsgFuncId_t>(msg_id && 0xFF);
        }

        /**
         * @brief Compile time method for creating a message ID.
         * 
         * @tparam TModId Module ID.
         * @tparam TCmdId Function ID.
         * @return constexpr auto MsgId_t
         */
        template <MsgModuleId_t TModId, MsgFuncId_t TCmdId>
        constexpr auto to_msg_id()
        {
            return static_cast<MsgId_t>((TModId << 8) | TCmdId);
        }

        /**
         * @brief Base for all messages.
         * 
         * @tparam TModId Module ID.
         * @tparam TFuncId Function ID.
         */
        template <MsgModuleId_t TModId, MsgFuncId_t TFuncId>
        struct BaseMsg : etl::message<to_msg_id<TModId, TFuncId>()>
        {};

        /**
         * @brief Function/command codes common to all apps/modules.
         * 
         */
        enum class CommonCmdCodes : MsgFuncId_t
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


        struct Msg : public etl::message<
        template <MsgModuleId_t TModId>
        struct WakeupMsg
        {
            /* data */
        };
        

    }
}
