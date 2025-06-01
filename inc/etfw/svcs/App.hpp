
#pragma once

#include "iSvc.hpp"
#include "SvcRegistry.hpp"
#include "msg/MsgBroker.hpp"
#include "Runner.hpp"
#include "SvcCfg.hpp"

namespace etfw {

    class iApp : public iSvc
    {
        public:
            /// TODO: probably need to refactor to some sort of list
            using ChildRegistry = SvcRegistry<iSvc, MAX_NUM_CHILD_SVCS>;
            using Status = iSvc::Status;

            /// @brief App interface constructor
            /// @param id Application ID
            /// @param name etl::string type of application name
            iApp(SvcId_t id, Name_t& name):
                iSvc(id, name) {}

            /// @brief App interface constructor
            /// @param id Application ID
            /// @param name Raw char name
            iApp(SvcId_t id, const char* name):
                iSvc(id, name) {}

            /// @brief Checks if a given child is registered
            /// @param child_id The requested child id.
            /// @return Registration state (TRUE = registered, FALSE = unregistered)
            inline bool is_child_registered(const SvcId_t child_id)
            {
                return Children.is_registered(child_id);
            }

            /// @brief Finds and returns child service
            /// @param child_id Child service ID
            /// @return Child service. Nullptr if unregistered.
            inline iSvc* get_child(const SvcId_t child_id) { return Children.find_svc(child_id); }

            /// @brief Get application children
            /// @return App children registry
            inline ChildRegistry& children() { return Children; }

            /// @brief Sends a command message to all subscribed applications
            /// @param msg Message to send
            static void send_cmd(const etl::imessage& msg);

        protected:
            /// @brief Registers and starts child service
            /// @param child Child service
            /// @return Registration/start status code
            Status start_child(iSvc& child);

            /// @brief Subscribes a message router to a set of commands
            /// @param msgs Subscription class containing the router and commands
            static void subscribe_cmd(Msg::Subscription &subscription);

            /// @brief Subscribes a message router to a set of commands
            /// @param handler The message router to subscribe
            /// @param msg_ids Initializer list of the commands IDs to subscribe to
            static void subscribe_cmd(etl::imessage_router& handler,
                std::initializer_list<Msg::MsgId> msg_ids);

            /// @brief Subscribes a message router to a set of commands
            /// @param handler The message router to subscribe
            /// @param msg_ids Run-time/dynamic container of command IDs
            static void subscribe_cmd(etl::imessage_router& handler,
                Msg::MsgIdContainer &msg_ids);

            /// @brief Subscribes a message router to a set of status messages
            /// @param msgs Subscription class containing the router and status messages
            static void subscribe_status(Msg::Subscription &msgs);

            /// @brief Subscribes a message router to a set of status messages
            /// @param handler The message router to subscribe
            /// @param msg_ids Initializer list of the status message IDs
            static void subscribe_status(etl::imessage_router& handler,
                std::initializer_list<Msg::MsgId> msg_ids);

            /// @brief Subscribes a message router to a set of commands
            /// @param handler The message router to subscribe
            /// @param msg_ids Run-time/dynamic container of status message IDs
            static void subscribe_status(etl::imessage_router& handler,
                Msg::MsgIdContainer &msg_ids);

        private:
            ChildRegistry Children;

            /// @brief Command broker/bus
            static Msg::Broker CmdBroker;

            /// @brief Status message broker/bus
            static Msg::Broker StatusBroker;

            /// @brief Wakeup broker/bus
            static Msg::Broker WakeupBroker;
    };


    /// @brief Policy-base CRTP application class. 
    /// @tparam Derived Derived application
    /// @tparam Cfg App configuration
    template <typename Derived, typename Cfg>
    class App : public iApp
    {
        //static_assert(
        //    std::is_base_of<Cfg, SvcCfg>::value,
        //    "Cfg template argument must derive from SvcCfg (found in SvcCfg.hpp)"
        //);
        //static_assert(
        //    valid_svc_name<Cfg>::value,
        //    "Cfg must have a valid NAME member"
        //);

        public:
            using Runner_t = typename Cfg::RunnerCfg::Runner_t;
            using RunState = RunStatus;

            static constexpr SvcId_t ID = Cfg::ID;

            App():
                iApp(Cfg::ID, Cfg::NAME),
                Runner(this)
            {}

            iSvcRunner* runner(void) override { return &Runner; }
            
            RunState process(void) override
            {
                return static_cast<Derived*>(this)->run_loop();
            }

            Status init_(void) override
            {
                return static_cast<Derived*>(this)->app_init();
            }

            Status start_() override
            {
                if (iSvcRunner::RunStatus::OK == Runner.start())
                {
                    log(LogLevel::INFO, "Started");
                }
                else
                {
                    log(LogLevel::ERROR, "Start failed\n");
                }
                return Status::Code::OK;
            }

            Status stop_(void) override
            {
                Runner.stop();
                return Status::Code::OK;
            }

            Status cleanup_(void) override
            {
                return static_cast<Derived*>(this)->app_cleanup();
            }

            bool is_init(void) const override
            {
                return true;
            }

            bool is_started(void) const override
            {
                return Runner.is_active();
            }
        
        protected:
            Runner_t Runner;
    };

}
