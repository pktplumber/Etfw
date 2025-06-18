
#pragma once

#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include "OsTypes.hpp"
#include "../Status.hpp"

namespace Os
{

using TaskSz = std::size_t;
constexpr std::size_t TaskNameSzMax = 32;
constexpr TimeMs_t TaskDefaultTimeout = 1000;

struct GlobTaskStats_t
{
    uint16_t TaskCount;
    uint16_t RunningTaskCount;

    GlobTaskStats_t():
        TaskCount(0),
        RunningTaskCount(0) {}
};

static inline GlobTaskStats_t GlobalTaskStats;

    struct TaskStack
    {
        using StackPtr_t = uint32_t*;

        const StackPtr_t StackBuf;
        const size_t Sz;

        TaskStack(StackPtr_t stack_buf, size_t stack_sz):
            StackBuf(stack_buf),
            Sz(stack_sz) {}
        
        TaskStack():
            StackBuf(nullptr),
            Sz(0) {}
    };


    class iTask
    {
        public:
            typedef void (*routine_t)(void*);

            struct Config
            {
                using Priority_t = uint8_t;
                using routine_arg_t = void*;
                typedef void (*routine_t)(routine_arg_t);

                TaskStack Stack;
                Priority_t Priority;
                TimeMs_t DefaultTimeoutMs;
                routine_t Routine;
                routine_arg_t RoutineArg;

                Config():
                    Stack(),
                    Priority(0),
                    DefaultTimeoutMs(0),
                    Routine(nullptr),
                    RoutineArg(nullptr) {}

                Config(
                    TaskStack &stack_cfg,
                    Priority_t priority,
                    routine_t routine,
                    routine_arg_t arg
                ):
                    Stack(stack_cfg),
                    Priority(priority),
                    DefaultTimeoutMs(TaskDefaultTimeout),
                    Routine(routine),
                    RoutineArg(arg) {}

                Config(
                    TaskStack &stack_cfg,
                    Priority_t priority,
                    TimeMs_t default_timeout_ms,
                    routine_t routine,
                    routine_arg_t arg
                ):
                    Stack(stack_cfg),
                    Priority(priority),
                    DefaultTimeoutMs(default_timeout_ms),
                    Routine(routine),
                    RoutineArg(arg)
                {}
            };

            enum RunCode: int
            {
                TASK_STOPPED,
                TASK_RUN,
                TASK_EXIT,
                TASK_ERROR,
                TASK_PAUSED,
            };

            enum Status : int32_t
            {
                OP_OK,
                CREATE_ERR,
                INVALID_CONFIG,
            };

            iTask():
                Cfg()
            {}

            iTask(Config &cfg):
                Cfg(cfg) 
            {}

            void set_config(Config &cfg)
            {
                Cfg.Routine = cfg.Routine;
                Cfg.RoutineArg = cfg.RoutineArg;
            }

            Status start(void)
            {
                Status status = OP_OK;
                if (pthread_create(&_thread, nullptr, thread_func_wrapper, this) != 0)
                {
                    status = CREATE_ERR;
                }
                return status;
            }

            inline void sleep(const TimeMs_t ms)
            {
                usleep(ms*1000);
            }

            inline bool is_active(void) const
            {
                return (_RunCode == TASK_RUN);
            }

            inline void stop(void) { _RunCode = TASK_STOPPED; }
        
        private:
            Config Cfg;
            pthread_t _thread;
            volatile RunCode _RunCode;

            Status verify_cfg(void) const
            {
                if (Cfg.Stack.StackBuf == nullptr ||
                    Cfg.Stack.Sz == 0 ||
                    Cfg.Routine == nullptr)
                {
                    return Status::INVALID_CONFIG;
                }

                return OP_OK;
            }

            static void *thread_func_wrapper(void *obj)
            {
                iTask *task = static_cast<iTask*>(obj);
                task->Cfg.Routine(task->Cfg.RoutineArg);
                return nullptr;
            }
    };

    template <TaskSz STACK_SZ, TimeMs_t TIMEOUT_MS = TaskDefaultTimeout>
    class Task
    {
        enum TaskRunCode: int
        {
            TASK_STOPPED,
            TASK_RUN,
            TASK_EXIT,
            TASK_ERROR,
            TASK_PAUSED,
        };

    public:
        Task(): 
            _RunCode(TASK_STOPPED),
            NameSet(false) {GlobalTaskStats.TaskCount++;}

        Task(const char *name):
            _RunCode(TASK_STOPPED),
            NameSet(true)
        {
            strncpy(TaskName, name, sizeof(TaskName)-1);
            TaskName[sizeof(TaskName)-1] = '\0';
            GlobalTaskStats.TaskCount++;
        }

        /**
         * @brief Starts task execution.
         * 
         * @return true 
         * @return false 
         */
        bool start(void)
        {
                _RunCode = TASK_RUN;
                // Need to use pthread here. OSAL task does not support passing arg
                // into thread function, which is required for this project's modularity
                // goals.
                /// TODO: only works with POSIX compliant OSes. Need to consider if others are required.
                return (pthread_create(&_Thread, nullptr, task_run_helper, this) == 0);
        }

        /**
         * @brief Stops the task. The task will need to be restarted (with start()) to run again.
         */
        inline void stop(void) { _RunCode = TASK_STOPPED; }

        /**
         * @brief Temporarily pauses task execution. Task will sleep continuously for TIMEOUT_MS until resumed.
         */
        inline void pause(void) { _RunCode = TASK_PAUSED; }

        /**
         * @brief 
         * 
         * @return true 
         * @return false 
         */
        inline bool is_active(void) const
        {
            return ((_RunCode == TASK_RUN) || (_RunCode == TASK_PAUSED));
        }

        /**
         * @brief 
         * 
         * @return true 
         * @return false 
         */
        inline bool is_running(void) const { return (_RunCode == TASK_RUN); }

        /**
         * @brief 
         * 
         * @return true 
         * @return false 
         */
        inline bool is_paused(void) const { return (_RunCode == TASK_PAUSED); }

        /**
         * @brief Returns the task name
         * 
         * @return const char* 
         */
        const char *name(void) const { return TaskName; }

    protected:
        TaskRunCode _RunCode;

        /**
         * @brief 
         * 
         */
        virtual void task_entry_init(void) {}

        virtual void task_logic(void)
        {
            _sleep(TIMEOUT_MS);
        }

        virtual void cleanup(void) {}

        void _sleep(const TimeMs_t ms) { usleep(ms*1000); }

    private:
        pthread_t _Thread;
        bool NameSet;
        char TaskName[TaskNameSzMax];

        static void *task_run_helper(void *task_obj)
        {
            Task *t = static_cast<Task*>(task_obj);
            t->task_loop();
            return NULL;
        }

        void task_loop(void)
        {
            task_entry_init();
            while (is_active())
            {
                if (is_running())
                {
                    task_logic();
                }
                else if (is_paused())
                {
                    _sleep(TIMEOUT_MS);
                }
            }
            cleanup();
        }
    };


    class Thread
    {
        public:
            struct ThreadOpStatus
            {
                enum class Code : int32_t
                {
                    OK = 0,
                    NOT_SUPPORTED,
                    INVALID_STACK_CFG,
                    INVALID_ROUTINE_CFG,
                    INVALID_PRIORITY_CFG,
                    THREAD_CREATE_ERROR,
                    INVALID_STATE,
                    JOIN_ERROR,
                };
                
                static constexpr StatusStr_t ErrStrLkup[] =
                {
                    "Success",
                    "Operation not supported"
                    "Invalid stack configuration",
                    "Invalid thread routine configuration",
                    "Invalid thread priority"
                    "Thread creation error"
                    "Invalid task state for operation"
                    "OS join operation failure"
                };
            };

            using Status = EtfwStatus<ThreadOpStatus>;
    
            enum class State
            {
                NOT_STARTED,
                RUNNING,
                EXITED
            };

        public:
            using RoutineArg_t = void*;
            typedef void (*Routine_t)(RoutineArg_t);

            struct Config
            {
                using Priority_t = uint8_t;

                struct Stack
                {
                    using Buf_t = uint32_t;
                    using BufPtr_t = Buf_t*;

                    const BufPtr_t Buf;
                    const size_t Sz;

                    Stack(BufPtr_t buf, size_t buf_sz):
                        Buf(buf),
                        Sz(buf_sz) {}
                };

                Stack StackBuf;
                const Priority_t Priority;
                const RoutineArg_t Arg;
                const Routine_t Routine;

                Config(Stack stack,
                    Priority_t priority,
                    RoutineArg_t arg,
                    Routine_t routine
                ):
                    StackBuf(stack),
                    Priority(priority),
                    Arg(arg),
                    Routine(routine) {}
                
                Config(Stack::BufPtr_t stack_buf,
                    size_t stack_buf_sz,
                    Priority_t priority,
                    RoutineArg_t arg,
                    Routine_t routine
                ):
                    StackBuf(stack_buf, stack_buf_sz),
                    Priority(priority),
                    Arg(arg),
                    Routine(routine) {}
            };

            Thread();

            Status start(Config& cfg);

            Status join(void);

            Status stop(void);

            static inline void delay(const TimeMs_t ms) { usleep(ms*1000); }

            inline State state(void) const { return state_; }

        private:
            pthread_t handle_;
            RoutineArg_t arg_;
            Routine_t routine_;
            volatile State state_;

            Status validate_config(Config& config);

            static void *runner(void *thread_obj);
    };

}
