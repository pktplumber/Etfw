
#pragma once

#include "Broker.hpp"

namespace etfw
{
    class iApp;
}

namespace etfw::msg
{
    /// @brief ETFW Message context interface.
    class Context
    {
    public:
        Context();

    protected:
        static inline void register_pipe(iPipe& pipe)
        {
            broker_.register_pipe(pipe);
        }

        static inline void unregister_pipe(iPipe& pipe)
        {
            broker_.unregister_pipe(pipe);
        }

        template <typename TMsg>
        static inline void send_msg(const TMsg& msg)
        {
            broker_.send(msg);
        }

        template <typename TMsg, typename... TArgs>
        static inline void send_msg(TArgs&&... args)
        {
            broker_.send<TMsg>(etl::forward<TArgs>(args)...);
        }

        static inline Buf* get_message_buf(const size_t buf_sz)
        {
            return broker_.get_message_buf(buf_sz);
        }

        static inline void send_buf(Buf& msg_buf)
        {
            broker_.send_buf(msg_buf);
        }

        static inline void return_message_buf(Buf* buf)
        {
            broker_.return_message_buf(buf);
        }

        static inline const MsgBufPool::Stats& msg_pool_stats()
        {
            return broker_.pool_stats();
        }

        static inline const Broker::Stats& broker_stats()
        {
            return broker_.stats();
        }
        
    private:
        static Broker broker_;
    };

    /// @brief Defines msg interface accessible by apps
    class AppContext : public Context
    {
    public:
        friend class etfw::iApp;

        // Expose app-accessible methods
        using Context::register_pipe;
        using Context::unregister_pipe;
        using Context::send_msg;
        using Context::get_message_buf;
        using Context::send_buf;
        using Context::return_message_buf;
        using Context::msg_pool_stats;
        using Context::broker_stats;
    };

    /// @brief Defines msg interface accessible by app children
    class ChildContext : public Context
    {
    protected:
        friend class iAppChild;

        // Expose app child accessible methods
        // Children can send messages and read stats, but they
        // they cannot receive messages directly
        using Context::send_msg;
        using Context::get_message_buf;
        using Context::send_buf;
        using Context::return_message_buf;
        using Context::msg_pool_stats;
        using Context::broker_stats;
    };

    /// @brief Defines msg interface accessible any object, typically
    ///     main loops/unit tests
    class GlobalContext : public Context
    {
    public:
        // Global objects (main loop) can only
        // send messages and read stats
        using Context::send_msg;
        using Context::msg_pool_stats;
        using Context::broker_stats;
    };

    extern GlobalContext glob;
    extern AppContext app_ctx;
    extern ChildContext child_ctx;
}
