
#pragma once

#include "Runner.hpp"

#include "etl/type_traits.h"

template <typename, typename = etl::void_t<>>
struct has_init_impl : etl::false_type {};

template <typename T>
struct has_init_impl<T, etl::void_t<decltype(etl::declval<T>().init_impl())>> : etl::true_type {};

template <typename, typename = etl::void_t<>>
struct has_cleanup_impl : etl::false_type {};

template <typename T>
struct has_cleanup_impl<T, etl::void_t<decltype(etl::declval<T>().cleanup_impl())>> : etl::true_type {};

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

        /// @brief Private initialization method. Calls derived init method
        /// @todo Investigate if fwd args can be used here
        /// @return Initialization status
        Status init_() override
        {
            // Call init implementation if implemented in derived class
            if constexpr (has_init_impl<Derived>::value)
            {
                return static_cast<Derived*>(this)->init_impl();
            }
            else
            {
                // Otherwise return OK
                return Status::Code::OK;
            }
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
            if constexpr (has_cleanup_impl<Derived>::value)
            {
                return static_cast<Derived*>(this)->cleanup_impl();
            }
            else
            {
                return Status::Code::OK;
            }
        }
    
    protected:
        Runner_t Runner;
    };
}
