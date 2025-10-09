
#include <etfw/svcs/App.hpp>
#include <etfw/msg/Pipe.hpp>
#include <etfw/msg/Context.hpp>

static constexpr etfw::SvcId_t AppId = 1;
static constexpr uint8_t AppPriority = 19;
static constexpr size_t AppStackSz = 1024;

using AppRunnerCfg = etfw::ActiveSvcCfg<AppPriority, AppStackSz>;

struct AppCfg : public etfw::SvcCfg<
    AppId, AppRunnerCfg>
{
    static constexpr const char* NAME = "EXAMPLE_APP";
};

class App : public etfw::App<App, AppCfg>
{
public:
    using Base_t = etfw::App<App, AppCfg>;
    using RunState = Base_t::RunState;
    using AppStatus = Base_t::Status;

    struct Msg1 : public etfw::msg::iBaseMsg
    {
        using Base_t = etfw::msg::iBaseMsg;

        Msg1():
            Base_t(1, sizeof(Msg1))
        {}
    };

    App():
        Base_t(),
        cmd_pipe_(*this, {1})
    {}

    AppStatus app_init()
    {
        comms.register_pipe(cmd_pipe_);
        return AppStatus::Code::OK;
    }

    RunState run_loop()
    {
        usleep(1000000);
        cmd_pipe_.process_queue(1000);
        log(etfw::LogLevel::INFO, "In app");
        return RunState::OK;
    }

    AppStatus app_cleanup()
    {
        return AppStatus::Code::OK;
    }

    void handle(const etl::imessage& msg)
    {
        log(etfw::LogLevel::INFO, "Got mid 0x%X", msg.get_message_id());
    }

private:
    using CmdPipe_t = etfw::msg::QueuedPipe<App, 10>;
    CmdPipe_t cmd_pipe_;
};


int main()
{
    App app;
    app.init();
    app.start();

    int iters = 5;
    while(true)
    {
        usleep(1000000);
        if (iters-- == 0)
        {
            printf("SENT CMD\n");
            App::Msg1 m1;
            //etfw::iApp::send_cmd(m1);
            etfw::msg::glob.send_msg(m1);
            iters = 5;
        }
    }
    return 0;
}
