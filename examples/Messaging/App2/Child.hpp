
#pragma once

#include <etfw/svcs/AppChild.hpp>
#include <etfw/svcs/SvcCfg.hpp>
#include "Msg.hpp"

namespace app2
{
    struct ChildCfg : public etfw::ChildSvcCfg<22, 1024>
    {};

    class Child : public etfw::AppChild<Child, ChildCfg>
    {
    public:
        using Base_t = etfw::AppChild<Child, ChildCfg>;

        Child():
            Base_t(1, "APP2_CHILD")
        {}

        inline void set_task_args(
            const uint32_t num_task_iters,
            const uint32_t task_period
        )
        {
            task_timeout_ = task_period;
            task_loops_remaining_ = num_task_iters;
        }

        RunState run_loop()
        {
            task_loops_remaining_--;
            usleep(task_timeout_*1000);
            log(etfw::LogLevel::INFO,
                "Iters remaining %d",
                task_loops_remaining_);
            if (task_loops_remaining_)
            {
                return RunState::OK;
            }

            return RunState::DONE;
        }

    private:
        uint32_t task_timeout_;
        uint32_t task_loops_remaining_;
    };
}
