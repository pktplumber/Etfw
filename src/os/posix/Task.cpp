
#include "os/Task.hpp"
#include <cassert>

using namespace Os;

Thread::Thread():
    state_(Thread::State::NOT_STARTED)
{}

Thread::Status Thread::start(Thread::Config& cfg)
{
    Thread::Status status = Thread::Status(Thread::Status::Code::INVALID_STATE);
    if (Thread::State::RUNNING != state_)
    {
        status = validate_config(cfg);
        if (Thread::Status::Code::OK == status.code())
        {
            arg_ = cfg.Arg;
            routine_ = cfg.Routine;
            int posix_result = pthread_create(&handle_, nullptr, runner, this);
            if (0 != posix_result)
            {
                return Thread::Status::Code::THREAD_CREATE_ERROR;
            }
            else
            {
                status = Thread::Status(Thread::Status::Code::OK);
            }
        }
    }

    return status;
}

Thread::Status Thread::join(void)
{
    Thread::Status status = Thread::Status(Thread::Status::Code::INVALID_STATE);
    if (Thread::State::RUNNING == state_)
    {
        Thread::Status status = Thread::Status::Code::JOIN_ERROR;
        int posix_result = pthread_join(handle_, nullptr);
        if (posix_result == 0)
        {
            status = Thread::Status::Code::OK;
        }
    }

    return status;
}

Thread::Status Thread::validate_config(Thread::Config& cfg)
{
    if (cfg.StackBuf.Buf == nullptr ||
        cfg.StackBuf.Sz == 0)
    {
        return Thread::Status::Code::INVALID_STACK_CFG;
    }

    if (cfg.Routine == nullptr)
    {
        return Thread::Status::Code::INVALID_ROUTINE_CFG;
    }

    return Thread::Status::Code::OK;
}

void* Thread::runner(void *thread_obj)
{
    assert(thread_obj != nullptr &&
        "Thread object must not be null");
    Thread* thread = static_cast<Thread*>(thread_obj);
    assert(thread->routine_ != nullptr &&
        "Thread routine cannot be null");
    thread->state_ = Thread::State::RUNNING;
    thread->routine_(thread->arg_);
    thread->state_ = Thread::State::EXITED;
    return nullptr;
}
