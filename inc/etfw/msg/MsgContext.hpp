
#pragma once

#include "Message.hpp"
#include "Broker.hpp"
#include "status.hpp"

namespace etfw {
namespace msg {

    /// @brief 
    class Context
    {
    public:
        /// @brief Status code type trait
        struct StatusTrait
        {
            enum class Code : uint32_t
            {
                OK,

                COUNT
            };

            static constexpr StatusStr_t ErrStrLkup[] =
            {
                "Success"
            };
        };

        /// @brief Status code type
        using Status = EtfwStatus<StatusTrait>;

        Context()
        {}

        Status send(const iBaseMsg& msg)
        {
            msg_broker_.receive(msg);
        }

        template <MsgModuleId_t ModIdV>
        Status send(const wakeup<ModIdV>& msg)
        {

        }

    private:
        Broker msg_broker_;
    };
}
}
