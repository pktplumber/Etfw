
#include "etfw/svcs/Executor.hpp"

static constexpr etfw::SvcId_t App2Id = 2;
static constexpr uint8_t App2Prior = 10;
static constexpr size_t App2StackSz = 1024;

static constexpr etfw::SvcId_t App4Id = 4;
static constexpr uint8_t App4Prior = 13;
static constexpr size_t App4StackSz = 1024;

static constexpr etfw::SvcId_t App5Id = 5;

static constexpr etfw::SvcId_t App6Id = 6;

struct App2Cfg : public etfw::SvcCfg<App2Id, etfw::ActiveSvcCfg<App2Prior, App2StackSz>>
{
    static constexpr const char* NAME = "APP_2";
};

struct App4Cfg : public etfw::SvcCfg<App4Id, etfw::ActiveSvcCfg<App4Prior, App4StackSz>>
{
    static constexpr const char* NAME = "APP_4";
};

template <typename TCfg>
class ExampleActiveApp : public etfw::App<ExampleActiveApp<TCfg>, TCfg>
{
    public:
        using Base_t = typename etfw::App<ExampleActiveApp<TCfg>, TCfg>;
        using Status = typename Base_t::Status;
        using RunState = typename Base_t::RunState;
        using Base_t::log;

        ExampleActiveApp():
            Base_t()
        {}

        Status app_init()
        {
            log(etfw::LogLevel::INFO, "App init");
            return Status::Code::OK;
        }

        RunState run_loop()
        {
            usleep(1000000);
            log(etfw::LogLevel::INFO, "In active app loop");
            return RunState::OK;
        }

        Status app_cleanup()
        {
            return Status::Code::OK;
        }
};

using App2 = ExampleActiveApp<App2Cfg>;
using App4 = ExampleActiveApp<App4Cfg>;

etfw::StaticExecutor<App2, App4> Exec;

int main()
{
    Exec.run();

    while(1)
    {}
    return 0;
}
