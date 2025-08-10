
#pragma once

#include <etl/vector.h>
#include "SvcTypes.hpp"
#include "../status.hpp"
//#include "iSvc.hpp"

namespace etfw
{
    class iSvc;


    /// Might be a good idea to move iSvcRegistry and SvcRegistry into iSvc class definition
    /// Currently facing circular dependency issue


    //class iSvcRegistry
    //{
    //    public:
    //        iSvcRegistry() {}
//
    //        virtual iSvc* data() = 0;
//
    //        virtual size_t size() = 0;
//
    //        iSvc* begin() {return data();}
    //        iSvc* end() {return data() + size();}
    //};

    template <typename T, size_t MaxNumSvcs>
    class SvcRegistry
    {
        /// TODO: Add static_assert to check that T derives from iSvc

        public:
            struct StatusTrait
            {
                enum class Code : int32_t
                {
                    OK,
                    REGISTRY_FULL,
                    UNREGISTERED_ERR,
                    ALREADY_REGISTERED,
                    UNKNOWN_REGISTRATION_ERROR,
                };

                static constexpr StatusStr_t ErrStrLkup[] =
                {
                    "Success",
                    "Registration error, registry full",
                    "Requested service ID is not registered",
                    "Registration error, service ID already registered"
                    "Unknown/ETL registration error"
                };
            };
            using Status = EtfwStatus<StatusTrait>;

            using SvcContainer_t = etl::vector<T*, MaxNumSvcs>;

            SvcRegistry(){}

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
                    return Status::Code::UNKNOWN_REGISTRATION_ERROR;
                }

                return Status::Code::OK;
            }

            Status unregister_svc(T& svc)
            {
                return unregister_svc(svc.id());
            }

            Status unregister_svc(const SvcId_t svc_id)
            {
                return Status::Code::UNKNOWN_REGISTRATION_ERROR;
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

            inline size_t num_svcs(void) const { return Svcs.size(); }

            inline SvcContainer_t& svc_container(void) { return Svcs; }

        private:
            SvcContainer_t Svcs;
            
    };

}
