
#include "Cfg.hpp"

namespace app2
{
    class App : public etfw::App<App, Cfg>
    {
        public:
            using Base_t = etfw::App<App, Cfg>;
            using Status = Base_t::Status;
            using RunState = Base_t::RunState;

            App():
                Base_t()
            {}

            Status app_init()
            {
                return Status::Code::OK;
            }

            RunState run_loop()
            {
                return RunState::OK;
            }

            Status app_cleanup()
            {
                return Status::Code::OK;
            }

        private:
            
    };
}
