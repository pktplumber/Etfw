
#include "svcs/Runner.hpp"
#include "svcs/iSvc.hpp"

using namespace etfw;

iSvcRunner::iSvcRunner():
    Svc(nullptr),
    State(State_t::CREATED)
{}

iSvcRunner::iSvcRunner(iSvc* svc):
    Svc(svc),
    State(State_t::CREATED)
{}

iSvcRunner::~iSvcRunner() = default;

void iSvcRunner::stop_children()
{
    if (Svc != nullptr)
    {
        iSvc::iRegistry *children = Svc->children();
        if (children != nullptr)
        {
            for (size_t i = 0; i < children->size(); i++)
            {
                children->data()[i]->stop();
            }
        }
    }
}

void iSvcRunner::log(const LogLevel lvl, const char* fmt, ...)
{
    va_list args;
    if (Svc != nullptr)
    {
        Svc->log(lvl, fmt, args);
    }
}

PassiveRunner::PassiveRunner(iSvc* svc):
    iSvcRunner(svc)
{}

iSvcRunner::RunStatus PassiveRunner::start()
{
    log(LogLevel::INFO, "Passive service started");
    Svc->pre_run_init();
    return iSvcRunner::RunStatus::OK;
}

iSvcRunner::RunStatus PassiveRunner::stop()
{
    stop_children();
    /// TODO: need some sort of wait/async call back for children to stop
    /// before calling cleanup
    Svc->post_run_cleanup();
    log(LogLevel::INFO, "Passive service stopped");
    return iSvcRunner::RunStatus::OK;
}

iActiveRunnerExt::iActiveRunnerExt(iSvc* svc,
    Priority_t priority,
    const StackBuf_t stack_buf,
    const size_t stack_sz
):
    iSvcRunner(svc),
    TaskPriority(priority),
    StackBuf(stack_buf),
    StackSz(stack_sz)
{
    ETFW_ASSERT(stack_buf != nullptr,
        "Attempt to create active runner with null stack buffer");
    ETFW_ASSERT(stack_sz > 0,
        "Attempt to create active runner with invalid stack size");
}

iSvcRunner::RunStatus iActiveRunnerExt::start()
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
            StackBuf,
            StackSz,
            TaskPriority,
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

iSvcRunner::RunStatus iActiveRunnerExt::stop()
{
    RunStatus status = RunStatus::DONE;
    if (State_t::ACTIVE == State)
    {
        State = State_t::STOP_REQUESTED;
        status = RunStatus::OK;
    }
    return status;
}

void iActiveRunnerExt::task_sm(void* runner)
{
    ETFW_ASSERT(runner != nullptr, "Null runner passed into task_sm");
    iActiveRunnerExt* runner_ = static_cast<iActiveRunnerExt*>(runner);
    task_sm_start(runner_);
    task_sm_run(runner_);
    task_sm_finish(runner_);
}

void iActiveRunnerExt::task_sm_start(iActiveRunnerExt* task)
{
    if (State_t::STARTING == task->State)
    {
        iSvc::RunStatus stat = task->Svc->pre_run_init();
        if (iSvc::RunStatus::OK == stat)
        {
            task->log(LogLevel::INFO,
                "Task context initialization complete. Starting active runner");
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

void iActiveRunnerExt::task_sm_run(iActiveRunnerExt* task)
{
    task->log(LogLevel::INFO,
        "ActiveRunner initialization complete. Starting task");
    while (State_t::ACTIVE == task->State)
    {
        iSvc::RunStatus stat = task->Svc->process();
        if (iSvc::RunStatus::DONE == stat)
        {
            task->log(LogLevel::INFO,
                "Service returned DONE. Exiting.");
            task->stop_children();

            stat = task->Svc->post_run_cleanup();
            if (iSvc::RunStatus::OK == stat ||
                iSvc::RunStatus::DONE == stat)
            {
                task->log(LogLevel::INFO,
                    "ActiveRunner exit cleanup complete. Service stopped.");
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
}

void iActiveRunnerExt::task_sm_finish(iActiveRunnerExt* task)
{
    if (State_t::STOP_REQUESTED == task->State)
    {
        task->log(LogLevel::INFO,
            "Stop requested. Exiting active runner service");
        task->State = State_t::STOPPING;
        task->stop_children();

        /// TODO: need some sort of wait/async call back for children to stop
        /// before calling cleanup

        iSvc::RunStatus stat = task->Svc->post_run_cleanup();
        if (iSvc::RunStatus::OK == stat ||
            iSvc::RunStatus::DONE == stat)
        {
            task->State = State_t::STOPPED;
            task->log(LogLevel::INFO,
                "Service stopped");
        }
        else
        {
            task->State = State_t::ERROR;
        }
    }
}
