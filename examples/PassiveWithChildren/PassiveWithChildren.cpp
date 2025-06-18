
#include "etfw/svcs/App.hpp"
#include "etfw/svcs/SvcCfg.hpp"
#include "etfw/svcs/AppChild.hpp"
#include "etfw/svcs/msg/Router.hpp"

static constexpr etfw::SvcId_t AppId = 1;

static constexpr etfw::SvcId_t Child1Id = 1;
static constexpr uint8_t Child1Priority = 22;
static constexpr size_t Child1StackSz = 1024;

static constexpr etfw::SvcId_t Child2Id = 2;
static constexpr uint8_t Child2Priority = 23;
static constexpr size_t Child2StackSz = 1024;

/// @brief Passive application configuration
struct AppCfg : public etfw::SvcCfg<AppId, etfw::PassiveSvcCfg>
{
    static constexpr const char* NAME = "EXAMPLE_APP";
};

/// @brief App child 1 configuration
struct Child1Cfg : public etfw::ChildSvcCfg<Child1Priority, Child1StackSz>
{
    static constexpr const char* NAME = "APP_CHILD_1";
    static constexpr uint32_t SLEEP_TIME = 1000000; // 1 second sleep
};

/// @brief App child 2 configuration
struct Child2Cfg : public etfw::ChildSvcCfg<Child2Priority, Child2StackSz>
{
    static constexpr const char* NAME = "APP_CHILD_2";
    static constexpr uint32_t SLEEP_TIME = 330000; // ~1/3 second sleep
};

/// @brief Command to start child svc 2
struct StartChild2Cmd : etl::message<1>{};


/// @brief Generic app child class. Prints while in loop
/// @tparam CFG Child configuration
template <typename CFG>
class AppChild : public etfw::AppChild<AppChild<CFG>, CFG>
{
    public:
        using Base_t = typename etfw::AppChild<AppChild<CFG>, CFG>;
        using RunState = typename Base_t::RunState;
        using SvcStatus = typename Base_t::Status;
        using Base_t::log;

        AppChild(uint8_t id):
            Base_t(id, CFG::NAME),
            loop_iter(0)
        {}

        /// @brief Child service task loop. Must do some form of blocking.
        /// @return 
        RunState run_loop()
        {
            usleep(CFG::SLEEP_TIME);
            loop_iter++;
            log(etfw::LogLevel::INFO, "In loop. ITER = %d", loop_iter);
            return RunState::OK;
        }
    
    private:
        int loop_iter;
};

/// @brief Passive application
class ExampleApp : public etfw::App<ExampleApp, AppCfg>
{
    public:
        using Base_t = etfw::App<ExampleApp, AppCfg>;
        using RunState = Base_t::RunState;
        using AppStatus = Base_t::Status;

        ExampleApp():
            Base_t(),
            child1(Child1Id),
            child2(Child2Id),
            msg_rtr(*this) // Need to pass handler (this object) to router
        {}

        /// @brief Subscribe to command and initialize each child service
        /// @return OK
        AppStatus app_init()
        {
            // Message router has a default subscription of message
            // template args
            subscribe_cmd(msg_rtr.subscription());

            AppStatus stat = child1.init();
            log(etfw::LogLevel::INFO, "Child 1 init status: %s",
                stat.str());
            stat = child2.init();
            log(etfw::LogLevel::INFO, "Child 2 init status: %s",
                stat.str());
            return AppStatus::Code::OK;
        }

        /// @brief Start child service #1
        /// @return OK
        RunState pre_run_init()
        {
            log(etfw::LogLevel::INFO, "Service entry called");
            AppStatus stat = start_child(child1);
            log(etfw::LogLevel::INFO, "Child 1 start status: %s",
                stat.str());
            return RunState::OK;
        }

        /// @brief Example of a passive app wakeup handler. 
        /// @return Run status
        RunState run_loop()
        {
            num_loops++;
            RunState run_state = RunState::OK;
            
            // Exit after 3 iterations
            if (num_loops == 3)
            {
                run_state = RunState::DONE;
            }

            return run_state;
        }

        /// @brief App service cleanup. Running child services are stopped by app runner automatically.
        /// @return DONE
        RunState post_run_cleanup()
        {
            log(etfw::LogLevel::INFO, "Service exit called");
            return RunState::DONE;
        }

        /// @brief Cleanup child services
        /// @return OK
        AppStatus app_cleanup()
        {
            log(etfw::LogLevel::INFO, "Initiating cleanup");
            AppStatus stat = child1.cleanup();
            log(etfw::LogLevel::INFO, "Child 1 cleanup status: %s",
                stat.str());
            stat = child2.cleanup();
            log(etfw::LogLevel::INFO, "Child 2 cleanup status: %s",
                stat.str());
            // Default == ok
            return AppStatus();
        }

        void receive(const StartChild2Cmd& cmd)
        {
            AppStatus stat = start_child(child2);
            if (!stat.success())
            {
                log(etfw::LogLevel::ERROR,
                    "Failed to start child service 2 (%s)",
                    stat.str());
            }
        }
    
    private:
        int num_loops = 0;
        AppChild<Child1Cfg> child1;
        AppChild<Child2Cfg> child2;

        etfw::Msg::Router<ExampleApp, 1, StartChild2Cmd> msg_rtr;
};

int main()
{
    ExampleApp app;
    app.init();
    app.start();

    int num_app_iters = 4;
    while(num_app_iters--)
    {
        usleep(1000000);
        app.process();
    }

    etfw::iApp::send_cmd(StartChild2Cmd{});

    num_app_iters = 4;
    while(num_app_iters--)
    {
        usleep(1000000);
        app.process();
    }

    app.stop();
    usleep(1000000);
    app.cleanup();

    return 0;
}
