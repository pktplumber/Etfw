
#pragma once

#include <cstdint>
#include <etl/string.h>
#include <Status.hpp>
#include "SvcTypes.hpp"
#include "SvcRegistry.hpp"
#include "log/Logger.hpp"

#ifndef COMPONENT_MAX_NAME_LEN
#define COMPONENT_MAX_NAME_LEN  24
#endif

#ifndef MAX_NUM_PARENT_SVCS
#define MAX_NUM_PARENT_SVCS     32
#endif

#ifndef MAX_NUM_CHILD_SVCS
#define MAX_NUM_CHILD_SVCS  16
#endif

namespace etfw
{

class iSvcRunner;

class iSvc
{
    public:
        /// @brief Service class status code trait
        struct SvcStatusTrait
        {
            /// @brief Status code definitions
            enum class Code : int32_t
            {
                OK,                 //< Operation success.
                ID_TAKEN,           //< Svc ID is taken.
                UNREGISTERED_ERR,   //< Svc unregistered.
                REGISTRY_FULL,      //< Svc registry full.
                UNINIT_ERR,         //< Svc uninitialized error.
                ALREADY_INIT,       //< Svc reinitialization error.
                ALREADY_STARTED,    //< Svc started error.
                STOPPED,            //< Svc stopped error.
                OS_ERR,             //< General OS method error.
                REREGISTRATION_ERR, //< 
                NO_MEM,             //< 
                UNKNOWN_ERR,        //< 
                INVALID_STATE,      //< 
            };

            /// @brief Status code string messages
            static constexpr StatusStr_t ErrStrLkup[] =
            {
                "Operation success",
                "Svc ID is taken",
                "Svc unregistered",
                "Svc registry full",
                "Svc uninitialized error",
                "Svc reinitialization error",
                "Svc started error",
                "Svc stopped error",
                "General OS method error"
                "Child service is already registered"
                "No memory available for operation",
                "Unknown error",
                "Operation not allowed in the current state"
            };
        };

        using SvcId_t = uint8_t;
        using Name_t = etl::string<COMPONENT_MAX_NAME_LEN>;

        /// @brief Svc op status type
        using Status = EtfwStatus<SvcStatusTrait>;

        /// @brief Service runner operation state
        enum class RunStatus
        {
            OK,
            DONE,
            ERROR,
        };

        /// @brief Initialize the component data + objects. Must be called before start.
        /// @return Svc status
        Status init();

        /// @brief Start the service. Service must be initialized.
        /// @return Svc status
        Status start();

        /// @brief Stops the service runner. Must be started.
        /// @return Svc status
        Status stop();

        /// @brief Uninitializes/cleans up service data. Service must not be started.
        /// @return Svc status
        Status cleanup();

        /// @brief Gets the service ID. 
        /// @return Service ID.
        inline SvcId_t id() const { return Id; }

        /// @brief Name string getter 
        /// @return Service name (etl::string)
        inline const Name_t& name() const { return Name; }

        /// @brief Get the raw (const char*) service name
        /// @return Service name
        inline const char* name_raw() const { return Name.data(); }

        /// @brief Service entry function. Called by runner when started.
        /// @return Service run state
        /// @retval [OK] Service will continue to main/periodic process.
        /// @retval [DONE] Service has finished tasks and will exit without error.
        /// @retval [ERROR] Service has encountered an error and must exit.
        virtual RunStatus pre_run_init() { return RunStatus::OK; }

        /// @brief Service process function. Called periodically by runner.
        /// @return Service run state
        /// @retval [OK] Service will continue to run.
        /// @retval [DONE] Service has finished tasks and will exit without error.
        /// @retval [ERROR] Service has encountered an error and must exit.
        virtual RunStatus process() { return RunStatus::DONE; }

        /// @brief Service exit function. Called upon successful exit/stop.
        /// @return Service run state
        /// @retval [OK] Service cleanup successful and will exit without error
        /// @retval [DONE] Service cleanup successful and will exit without error
        /// @retval [ERROR] Service has encountered a cleanup error.
        virtual RunStatus post_run_cleanup() { return RunStatus::OK; }

        /// @brief Returns the service's runner object
        /// @return Pointer to service runner.
        virtual iSvcRunner* runner() = 0;

        /// @brief Implementation-specific initialization method for service.
        /// @return Svc status
        virtual Status init_() = 0;

        /// @brief Implementation-specific start method for service.
        ///     Typically will call service runner start method
        /// @return Svc status
        virtual Status start_() = 0;

        /// @brief Implementation-specific stop method for service.
        ///     Typically will call service runner stop method
        /// @return Svc status
        virtual Status stop_() = 0;

        /// @brief Implementation-specific cleanup method for service.
        /// @return Svc status
        virtual Status cleanup_() = 0;

    protected:
        /// @brief Construct a new Svc object
        /// 
        /// @param id Service Id
        /// @param name Service name
        iSvc(SvcId_t id, Name_t& name);

        /// @brief Construct a new Svc object
        /// 
        /// @param id Service Id
        /// @param name Service name
        iSvc(SvcId_t id, const char* name);

        /// @brief Formats and logs a service message. 
        /// @param level Log level
        /// @param format String/format to log.
        /// @param Args String format arguments
        void log(const LogLevel level, const char* format, ...);
    
    private:
        SvcId_t Id;
        Name_t Name;

        volatile bool IsInit;
        volatile bool IsStarted;
};

}
