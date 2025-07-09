
#pragma once

#include "os/Task.hpp"
#include "etfw_assert.hpp"
#include "iSvc.hpp"

/// Time to wait between checking if child tasks have exited
#ifndef ETFW_TASK_CLEANUP_DELAY
#define ETFW_TASK_CLEANUP_DELAY     1000
#endif

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
                return (!is_active());
            }

            void stop_children();

        protected:
            iSvc* Svc;
            State_t State;

            /// @brief Interface to Svc logger.
            /// @param level Log severity level
            /// @param format String to format and write
            /// @param Args String format arguments
            void log(const LogLevel level, const char* format, ...);
    };


    class PassiveRunner : public iSvcRunner
    {
        public:
            PassiveRunner(iSvc* svc);

            RunStatus start() override;
            RunStatus stop() override;
    };

    class iActiveRunnerExt : public iSvcRunner
    {
        public:
            using Priority_t = Os::Thread::Config::Priority_t;
            using Stack_t = Os::Thread::Config::Stack::Buf_t;
            using StackBuf_t = Os::Thread::Config::Stack::BufPtr_t;

            iActiveRunnerExt(iSvc* svc,
                Priority_t priority,
                const StackBuf_t stack_buf,
                const size_t stack_sz
            );

            /**
             * @brief Verifies the current state and starts the service runner thread.
             * 
             * @return RunStatus 
             */
            RunStatus start() override;

            /**
             * @brief Stops the 
             * 
             * @return RunStatus 
             */
            RunStatus stop() override;

        private:
            Priority_t TaskPriority;
            const StackBuf_t StackBuf;
            const size_t StackSz;
            Os::Thread task_;

            /**
             * @brief Main task state machine. Called in OS task.
             * 
             * @param runner 
             */
            static void task_sm(void* runner);

            /**
             * @brief Handles startup/initialization sequence for runner
             * 
             * @param task 
             */
            static void task_sm_start(iActiveRunnerExt* task);

            /**
             * @brief Main loop for service runner
             * 
             * @param task 
             */
            static void task_sm_run(iActiveRunnerExt* task);

            /**
             * @brief Handles cleanup when runner is stopped
             * 
             * @param task 
             */
            static void task_sm_finish(iActiveRunnerExt* task);
            
    };

    template <size_t TStackSz>
    class iActiveRunner : public iActiveRunnerExt
    {
        public:
            using Base_t = iActiveRunnerExt;

            iActiveRunner(iSvc* svc,
                Priority_t priority
            ):
                Base_t(svc, priority, Stack, TStackSz)
            {}

        private:
            Base_t::Stack_t Stack[TStackSz];
    };

    template <iActiveRunnerExt::Priority_t TPriority, size_t TStackSz>
    class ActiveRunner : public iActiveRunner<TStackSz>
    {
        public:
            using Base_t = iActiveRunner<TStackSz>;

            ActiveRunner(iSvc* svc):
                Base_t(svc, TPriority)
            {}
    };

}
