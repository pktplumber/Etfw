
#include "etfw/svcs/App.hpp"
#include "etfw/svcs/Executor.hpp"
#include "etfw/svcs/SvcCfg.hpp"
#include "etfw/svcs/log/Logger.hpp"

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

struct App5Cfg : public etfw::SvcCfg<App5Id, etfw::PassiveSvcCfg>
{
    static constexpr const char* NAME = "APP_5";
};

struct App6Cfg : public etfw::SvcCfg<App6Id, etfw::PassiveSvcCfg>
{
    static constexpr const char* NAME = "APP_6";
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

template <typename TCfg>
class ExamplePassiveApp : public etfw::App<ExamplePassiveApp<TCfg>, TCfg>
{
    public:
        using Base_t = typename etfw::App<ExamplePassiveApp<TCfg>, TCfg>;
        using Status = typename Base_t::Status;
        using RunState = typename Base_t::RunState;
        using Base_t::log;

        ExamplePassiveApp():
            Base_t()
        {}

        Status app_init()
        {
            return Status::Code::OK;
        }

        RunState run_loop()
        {
            log(etfw::LogLevel::INFO, "In passive app loop");
            return RunState::OK;
        }

        Status app_cleanup()
        {
            return Status::Code::OK;
        }
};

using Executor = etfw::Executor<20>;

int main()
{
    ExampleActiveApp<App2Cfg> app2;
    ExampleActiveApp<App4Cfg> app4;
    ExamplePassiveApp<App5Cfg> app5;
    ExamplePassiveApp<App6Cfg> app6;

    Executor exec;
    Executor::Status stat = exec.register_app(app2);
    EtfLog::log((stat.success() ? etfw::LogLevel::INFO : etfw::LogLevel::ERROR),
        "MAIN", "Register App2 result = %s", stat.str());
    stat = exec.register_app(app4);
    EtfLog::log((stat.success() ? etfw::LogLevel::INFO : etfw::LogLevel::ERROR),
        "MAIN", "Register App4 result = %s", stat.str());
    stat = exec.register_app(app5);
    EtfLog::log((stat.success() ? etfw::LogLevel::INFO : etfw::LogLevel::ERROR),
        "MAIN", "Register App5 result = %s", stat.str());
    stat = exec.register_app(app6);
    EtfLog::log((stat.success() ? etfw::LogLevel::INFO : etfw::LogLevel::ERROR),
        "MAIN", "Register App6 result = %s", stat.str());
    
    stat = exec.start_all();
    EtfLog::log((stat.success() ? etfw::LogLevel::INFO : etfw::LogLevel::ERROR),
        "MAIN", "Start all result = %s", stat.str());
    
    int iters = 20;
    while (iters--)
    {
        usleep(1000000);
        if (iters % 4 == 0)
        {
            app5.run_loop();
        }
        if (iters % 5 == 0)
        {
            app6.run_loop();
        }
    }

    exec.stop_all();
    usleep(2000000);
    EtfLog::log(etfw::LogLevel::INFO, "MAIN", "Exiting");
    return 0;
}
