
#pragma once

#include "os/Task.hpp"
#include "etfw_assert.hpp"
#include "iSvc.hpp"

namespace etfw {

class iSvcRunner
{
    public:
        enum class State_t
        {
            CREATED,        /**< Service object has been created. */
            INITIALIZED,    /**< Service runner has been initialized. */
            STARTING,       /**< Service has been requested to start. */
            ACTIVE,         /**< Service has started an is running. */
            STOP_REQUESTED, /**< Service has been requested to stop running. */
            STOPPING,       /**< Service is stopping. */
            STOPPED,        /**< Service has been stopped successfully. */
            EXITED,         /**< Service has exited (initiated internally). */
            ERROR,          /**< Service init, start, or stop has failed 
                                 or has encountered an internal error. */
        };
        
        enum class RunStatus
        {
            OK,
            DONE,
            ERROR,
        };

        iSvcRunner();

        iSvcRunner(iSvc* svc);

        virtual ~iSvcRunner();

        virtual RunStatus start() = 0;

        virtual RunStatus stop() = 0;

        inline State_t state() const { return State; }

        inline iSvc* svc() { return Svc; }

        inline bool is_active(void) const
        {
            return ((State == State_t::STARTING) || 
                    (State == State_t::ACTIVE));
        }

        inline bool is_stopped(void) const
        {
            return ((State == State_t::CREATED) ||
                    (State == State_t::INITIALIZED) ||
                    (State == State_t::STOPPED) ||
                    (State == State_t::EXITED) ||
                    (State == State_t::ERROR));
        }

    protected:
        iSvc* Svc;
        State_t State;
};


class PassiveRunner : public iSvcRunner
{
    public:
        PassiveRunner(iSvc* svc);

        RunStatus start() override;
        RunStatus stop() override;
};


template <Os::Thread::Config::Priority_t TPriority, size_t TStackSz>
class ActiveRunner : public iSvcRunner
{
    public:
        using Priority_t = Os::Thread::Config::Priority_t;
        using Stack_t = Os::Thread::Config::Stack::Buf_t;
        using StackBuf_t = Os::Thread::Config::Stack::BufPtr_t;
        static constexpr size_t StackSz = TStackSz;
        static constexpr Priority_t Priority = TPriority;

        ActiveRunner(iSvc* svc):
            iSvcRunner(svc)
        {}

        /**
         * @brief Verifies the current state and starts the service runner thread.
         * 
         * @return RunStatus 
         */
        RunStatus start() override
        {
            RunStatus run_status = RunStatus::ERROR;

            if (State_t::CREATED == State ||
                State_t::INITIALIZED == State ||
                State_t::EXITED == State ||
                State_t::STOPPED == State ||
                State_t::ERROR == State)
            {
                Os::Thread::Routine_t routine = this->task_sm;
                Os::Thread::RoutineArg_t arg = this;
                Os::Thread::Config task_cfg(
                    Stack_,
                    StackSz,
                    Priority,
                    arg,
                    routine
                );
                State = State_t::STARTING;
                Os::Thread::Status os_stat = task_.start(task_cfg);
                if (os_stat.error())
                {
                    State = State_t::ERROR;
                }
                else
                {
                    run_status = RunStatus::OK;
                }
            }

            return run_status;
        }

        /**
         * @brief Stops the 
         * 
         * @return RunStatus 
         */
        RunStatus stop() override
        {
            RunStatus status = RunStatus::DONE;
            printf("\nIN STOP\n");

            if (State_t::ACTIVE == State)
            {
                printf("State is active. Requesting stop\n");
                State = State_t::STOP_REQUESTED;
                status = RunStatus::OK;
            }
            else
            {
                printf("State is not active\n");
            }
            printf("\nLEAVING STOP\n");

            return status;
        }

    private:
        Stack_t Stack_[TStackSz];
        Os::Thread task_;

        static void task_sm(void* runner)
        {
            ETFW_ASSERT(runner != nullptr, "Null runner passed into task_sm");
            ActiveRunner* runner_ = static_cast<ActiveRunner*>(runner);
            task_sm_start(runner_);
            task_sm_run(runner_);
            task_sm_finish(runner_);
        }

        static void task_sm_start(ActiveRunner* task)
        {
            if (State_t::STARTING == task->State)
            {
                iSvc::RunStatus stat = task->Svc->pre_run_init();
                if (iSvc::RunStatus::OK == stat)
                {
                    task->State = State_t::ACTIVE;
                }
                else if (iSvc::RunStatus::DONE == stat)
                {
                    task->State = State_t::EXITED;
                }
                else
                {
                    task->State = State_t::ERROR;
                }
            }
        }

        static void task_sm_run(ActiveRunner* task)
        {
            while (State_t::ACTIVE == task->State)
            {
                iSvc::RunStatus stat = task->Svc->process();
                if (iSvc::RunStatus::DONE == stat)
                {
                    stat = task->Svc->post_run_cleanup();
                    if (iSvc::RunStatus::OK == stat ||
                        iSvc::RunStatus::DONE == stat)
                    {
                        task->State = State_t::EXITED;
                    }
                    else
                    {
                        // Failure
                    }
                }
                else if (iSvc::RunStatus::ERROR == stat)
                {
                    task->State = State_t::ERROR;
                }
            }
            printf("Exiting process");
        }

        static void task_sm_finish(ActiveRunner* task)
        {
            if (State_t::STOP_REQUESTED == task->State)
            {
                iSvc::RunStatus stat = task->Svc->post_run_cleanup();
                if (iSvc::RunStatus::OK == stat ||
                    iSvc::RunStatus::DONE == stat)
                {
                    task->State = State_t::STOPPED;
                }
                else
                {
                    task->State = State_t::ERROR;
                }
            }
        }
};

}
