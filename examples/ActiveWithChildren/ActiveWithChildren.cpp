
#include "etfw/svcs/App.hpp"
#include "etfw/svcs/SvcCfg.hpp"
#include "etfw/svcs/AppChild.hpp"
#include "etfw/svcs/msg/Router.hpp"

static constexpr etfw::SvcId_t AppId = 1;
static constexpr uint8_t AppPriority = 19;
static constexpr size_t AppStackSz = 1024;

static constexpr etfw::SvcId_t Child1Id = 1;
static constexpr uint8_t Child1Priority = 22;
static constexpr size_t Child1StackSz = 1024;

static constexpr etfw::SvcId_t Child2Id = 2;
static constexpr uint8_t Child2Priority = 23;
static constexpr size_t Child2StackSz = 1024;


struct AppCfg : public etfw::SvcCfg<AppId, etfw::ActiveSvcCfg<AppPriority, AppStackSz>>
{
    static constexpr const char* NAME = "EXAMPLE_APP";
};

struct Child1Cfg : public etfw::ChildSvcCfg<Child1Priority, Child1StackSz>
{};

/// @brief App child 2 configuration
struct Child2Cfg : public etfw::ChildSvcCfg<Child2Priority, Child2StackSz>
{};

/// @brief One shot child task. Loops 5 times and exits
class Child1 : public etfw::AppChild<Child1, Child1Cfg>
{
    public:
        using Base_t = etfw::AppChild<Child1, Child1Cfg>;
        using RunState = Base_t::RunState;
        using SvcStatus = Base_t::Status;

        Child1():
            Base_t(Child1Id, "CHILD_1")
        {}

        RunState run_loop()
        {
            iters_left--;
            log(etfw::LogLevel::INFO, "Iterations left = %d", iters_left);

            usleep(500000);

            RunState run_state = RunState::OK;
            if (iters_left == 0)
            {
                run_state = RunState::DONE;
            }

            return run_state;
        }
    
        private:
            int iters_left = 5;
};

class Child2 : public etfw::AppChild<Child2, Child2Cfg>
{
    public:
        static constexpr uint8_t ID = 22;
        using Base_t = etfw::AppChild<Child2, Child2Cfg>;
        using RunState = Base_t::RunState;
        using SvcStatus = Base_t::Status;

        struct LocalCmd : public etl::message<1>
        {
            uint16_t Num;

            LocalCmd(uint16_t num): Num(num) {}
        };

        Child2():
            Base_t(Child2Id, "CHILD_2"),
            local_m_handler(*this)
        {}

        RunState run_loop()
        {
            local_m_handler.process_msg_queue(1000);
            return RunState::OK;
        }

        void receive(const LocalCmd& cmd)
        {
            log(etfw::LogLevel::INFO, "Got local command. Num = %d", cmd.Num);
        }

        void send_cmd(const etl::imessage& cmd)
        {
            local_m_handler.receive(cmd);
        }

    private:
        etfw::Msg::QueuedRouter<Child2, 2, LocalCmd> local_m_handler;
};

class ExampleApp : public etfw::App<ExampleApp, AppCfg>
{
    public:
        using Base_t = etfw::App<ExampleApp, AppCfg>;
        using RunState = Base_t::RunState;
        using AppStatus = Base_t::Status;

        struct StartChildSvc : public etl::message<1>
        {
            uint8_t SvcId;

            StartChildSvc(): SvcId(0) {}
            StartChildSvc(uint8_t svc_id): SvcId(svc_id) {}
        };

        struct StopChildSvc : public etl::message<2>
        {
            uint8_t SvcId;

            StopChildSvc(): SvcId(0) {}
            StopChildSvc(uint8_t svc_id): SvcId(svc_id) {}
        };

        struct CommunicateWithChild : public etl::message<3>
        {
            uint16_t Num;

            CommunicateWithChild(uint16_t num): Num(num) {}
        };

        ExampleApp():
            Base_t(),
            cmd_handler(*this)
        {}

        AppStatus app_init()
        {
            subscribe_cmd(cmd_handler.subscription());
            child1.init();
            child2.init();
            
            return AppStatus::Code::OK;
        }

        RunState run_loop()
        {
            // Blocking. Will automatically call command handlers as messages
            // are dequeued
            cmd_handler.process_msg_queue(1000);
            log(etfw::LogLevel::INFO, "Processed queue");
            return RunState::OK;
        }

        AppStatus app_cleanup()
        {
            child1.cleanup();
            child2.cleanup();
            return AppStatus::Code::OK;
        }

        void receive(const StartChildSvc& cmd)
        {
            log(etfw::LogLevel::INFO, "Got StartChildSvc command for svc ID %d",
                cmd.SvcId);
            switch (cmd.SvcId)
            {
            case Child1Id:
                start_child(child1);
                break;
            
            case Child2Id:
                start_child(child2);
                break;
            
            default:
                break;
            }
        }

        void receive(const StopChildSvc& cmd)
        {
            log(etfw::LogLevel::INFO, "Got StopChildSvc command for svc ID %d",
                cmd.SvcId);
        }

        void receive(const CommunicateWithChild& cmd)
        {
            log(etfw::LogLevel::INFO, "Sending command to child2");
            child2.send_cmd(Child2::LocalCmd(cmd.Num));
        }
    
    private:
        etfw::Msg::QueuedRouter<ExampleApp, 5,
            StartChildSvc, StopChildSvc, CommunicateWithChild> cmd_handler;
        Child1 child1;
        Child2 child2;
};

int main()
{
    ExampleApp app;
    app.init();
    app.start();
    usleep(4000000);
    etfw::iApp::send_cmd(ExampleApp::StartChildSvc(1));
    usleep(7000000);
    etfw::iApp::send_cmd(ExampleApp::StartChildSvc(2));
    usleep(3000000);
    etfw::iApp::send_cmd(ExampleApp::CommunicateWithChild(99));
    usleep(3000000);
    app.stop();
    usleep(2000000);
    app.cleanup();
    return 0;
}
