
#pragma once

#include "Runner.hpp"

namespace etfw
{
    class iAppChild : public iSvc
    {
        public:
            iAppChild(SvcId_t id, const char* name):
                iSvc(id, name)
            {}
        
        private:
    };

    template <typename Derived, typename Cfg>
    class AppChild : public iAppChild
    {
        public:
            using Runner_t = typename Cfg::Runner_t;
            static constexpr SvcId_t ID = Cfg::ID;
            using RunState = RunStatus;

            AppChild(SvcId_t id, const char* name):
                iAppChild(id, name),
                Runner(this)
            {}

            iSvcRunner* runner(void) override { return nullptr; }
            
            RunState process(void) override
            {
                return static_cast<Derived*>(this)->run_loop();
            }

            Status init_(void) override
            {
                return Status::Code::OK;
            }

            Status start_() override
            {
                Status stat = Status::Code::OK;
                if (iSvcRunner::RunStatus::OK != Runner.start())
                {
                    stat = Status::Code::STOPPED;
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
                return Status::Code::OK;
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
