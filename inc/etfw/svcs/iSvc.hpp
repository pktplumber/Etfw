
#pragma once

#include <cstdint>
#include <etl/string.h>
#include "../Status.hpp"
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
                ALREADY_REGISTERED,
                UNKNOWN_REGISTRATION_ERR,
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

        class iRegistry
        {
            public:
                virtual iSvc** data() = 0;

                virtual size_t size() = 0;

                //iSvc* begin() {return data();}
                //iSvc* end() {return data() + size();}
        };

        template <typename T, size_t MaxNumSvcs>
        class Registry : public iRegistry
        {
            public:
                using SvcContainer_t = etl::vector<T*, MaxNumSvcs>;
                
                Registry(){}

                iSvc** data() override { return Svcs.data(); }
                size_t size() override { return Svcs.size(); }

                Status register_svc(T& svc)
                {
                    if (is_registered(svc.id()))
                    {
                        return Status::Code::ALREADY_REGISTERED;
                    }

                    size_t num_registered_svcs = Svcs.size();
                    if (num_registered_svcs == MaxNumSvcs)
                    {
                        return Status::Code::REGISTRY_FULL;
                    }

                    Svcs.push_back(&svc);
                    if (Svcs.size() != num_registered_svcs+1)
                    {
                        return Status::Code::UNKNOWN_REGISTRATION_ERR;
                    }

                    return Status::Code::OK;
                }

                T* find_svc(const SvcId_t svc_id)
                {
                    T* ret = nullptr;
                    for (auto &svc: Svcs)
                    {
                        if (svc->id() == svc_id)
                        {
                            ret = svc;
                            break;
                        }
                    }
                    return ret;
                }

                bool is_registered(T& svc)
                {
                    return is_registered(svc.id());
                }

                bool is_registered(const SvcId_t svc_id)
                {
                    if (find_svc(svc_id) == nullptr)
                    {
                        return false;
                    }
                    return true;
                }
            
            private:
                SvcContainer_t Svcs;
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

        /// @brief  Checks if the service has been initialized successfully.
        /// @return Initialization status
        /// @retval true The service is initialized and ready to run.
        /// @retval false The service is unitinialized
        inline bool is_init() const { return IsInit; }

        /// @brief  Checks if the service has been started/is active.
        /// @return Running status
        /// @retval true The service is active/running.
        /// @retval false The service is innactive/stopped.
        inline bool is_started() const { return IsStarted; }

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

        virtual iRegistry* children() { return nullptr; }

        friend class iSvcRunner;

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

        /// @brief Formats and logs a service message. 
        /// @param level Log level
        /// @param format String/format to log.
        /// @param Args String format arguments
        void log(const LogLevel level, const char* format, ...);

        /// @brief Default log method. Formats and logs a INFO service message.
        /// @param format String/format to log.
        /// @param Args String format arguments
        void log(const char* format, ...);
    
    private:
        SvcId_t Id;
        Name_t Name;

        volatile bool IsInit;
        volatile bool IsStarted;
};

}
