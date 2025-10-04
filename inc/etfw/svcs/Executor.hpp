
#pragma once

#include <cstdint>
#include <cstddef>
#include <variant>

#include <etl/variant.h>
#include <etl/visitor.h>
#include <etl/utility.h>

#include "CommonTraits.hpp"
#include "../status.hpp"
#include "SvcTypes.hpp"
#include "App.hpp"
#include "etl/list.h"

namespace etfw
{
    class iExecutor
    {
        public:
            struct ExecutorStatusTrait
            {
                enum class Code : int32_t
                {
                    OK,
                    ID_TAKEN,
                    REGISTRY_FULL,
                    UNKNOWN_REGISTRATION_ERR,
                    SVC_ALREADY_STARTED,
                    UNKNOWN_ID,
                    INITIALIZATION_ERR,
                    START_FAILURE,

                    COUNT
                };

                static constexpr StatusStr_t ErrStrLkup[] =
                {
                    "Success",
                    "ID already exists in registry",
                    "App registry full",
                    "Unknown/library registration error",
                    "Service is already running",
                    "Requested id is unregistered",
                    "Svc initialization error",
                    "Service start failure"
                };
            };

            struct Stats
            {
                size_t AppsRegistered;
                size_t AppsInitialized;
                size_t AppsActive;

                Stats():
                    AppsRegistered(0),
                    AppsInitialized(0),
                    AppsActive(0)
                {}
            };
            

            using Status = EtfwStatus<ExecutorStatusTrait>;
            using Node_t = iApp*;
            using iAppContainer = etl::ilist<Node_t>;

            /// @brief Registers an application. The executor will manage all registered applications.
            /// @param app Application to register
            /// @return 
            Status register_app(iApp& app);

            Status register_app(iApp* app);

            /// @brief Initializes and starts all registered apps.
            /// @details This method will initialize and start all registered apps. If an app
            ///          is already initialized, it skip initialization and immediately try to
            ///          start the app. Likewise, if an app has already been started, the start
            ///          sequence is skipped. If app initialization or start fails, an error
            ///          message will be logged, and the next app in the is processed.
            /// @return Operation status. Always successful.
            /// @retval Status::Code::OK
            Status start_all();

            Status start(const SvcId_t app_id);

            Status stop_all();

            Status stop(const SvcId_t app_id);

            Status run() { return start_all(); }

            Status exit() { return stop_all(); }

        protected:
            iExecutor(iAppContainer& apps, const size_t max_num_apps):
                Apps(apps),
                ContainerSize(max_num_apps)
            {}
            ~iExecutor() = default;
        
        private:
            iAppContainer& Apps;
            const size_t ContainerSize;

            Node_t find(const SvcId_t id);

            bool is_registered(const SvcId_t id);

            bool is_registered(const iApp& app);

            Status start_svc(iApp* app);
    };

    template <size_t MAX_NUM_APPS>
    class Executor : public iExecutor
    {
        public:
            using Base_t = iExecutor;

            Executor():
                Base_t(Apps, MAX_NUM_APPS)
            {}

        private:
            etl::list<Base_t::Node_t, MAX_NUM_APPS> Apps;
    };

    /// @brief Static storage and run-time manager for applications.
    /// @details AppExecutor stores, constructs, and application objects. Applications
    ///          must derive from the iApp class. Also, there is only one instance of
    //           each application type allowed in the template list arguments.
    /// @tparam ...TApps Default-constructable, iApp-derived types. No duplicate types allowed.
    template <typename... TApps>
    class AppExecutor : public Executor<sizeof...(TApps)>
    {
        static_assert(all_derived_from<iApp, TApps...>::value);

        public:
            using AppVariant_t = etl::variant<TApps...>;
            using AppStorage_t = etl::array<AppVariant_t, sizeof...(TApps)>;
            using Base_t = Executor<sizeof...(TApps)>;
            using Base_t::register_app;

            /// @brief AppExecutor constructor. Initializes and registers applications. 
            AppExecutor():
                AppStorage(init_storage(etl::make_index_sequence<sizeof...(TApps)>{}))
            {
                for (auto& app_variant: AppStorage)
                {
                    etl::visit([this](auto& app)
                    {
                        register_app(&app);
                    }, app_variant);
                }
            }

        private:
            static constexpr size_t NumApps = sizeof...(TApps);
            AppStorage_t AppStorage;

            /// @brief Constructs and returns an application in place
            /// @tparam T 
            /// @return 
            template <typename T>
            static AppVariant_t construct_var_in_place()
            {
                AppVariant_t var;
                var.template emplace<T>();  // construct in-place
                return var;
            }

            /// @brief Compile-time app storage builder
            /// @tparam ...Idxs Array indices
            template <size_t... Idxs>
            static AppStorage_t init_storage(etl::index_sequence<Idxs...>)
            {
                return {construct_var_in_place<TApps>()...};
            }
    };

} // namespace etfw

