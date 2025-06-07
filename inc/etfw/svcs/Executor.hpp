
#pragma once

#include <cstdint>
#include <cstddef>
#include <variant>
#include "CommonTraits.hpp"

#include "../Status.hpp"
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

            Status exit();
        
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
    /// @details StaticExecutor stores, constructs, and application objects. Applications
    ///          must derive from the iApp class. Also, there is only one instance of
    //           each application type allowed in the template list arguments.
    /// @tparam ...TApps Default-constructable, iApp-derived types. No duplicate types allowed.
    template <typename... TApps>
    class StaticExecutor : public Executor<sizeof...(TApps)>
    {
        static_assert(all_derived_from<iApp, TApps...>::value);

        public:
            using AppVariant_t = std::variant<TApps...>;
            using AppContainer = std::array<AppVariant_t, sizeof...(TApps)>;
            using Base_t = Executor<sizeof...(TApps)>;
            using Base_t::register_app;
            using Base_t::start_all;
            using Status = typename Base_t::Status;

            StaticExecutor():
                AppStorage(init_storage(std::index_sequence_for<TApps...>{}))
            {}

            void run()
            {
                for (auto& app_variant: AppStorage)
                {
                    std::visit([this](auto& app)
                    {
                        Status stat = register_app(&app);
                    }, app_variant);
                }

                start_all();
            }

            void register_all()
            {
                for (auto& var: AppStorage)
                {
                    std::visit([this](auto& app)
                    {
                        register_app(&app);
                    }, var);
                }
            }

            void test()
            {
                for (auto& var: AppStorage)
                {
                    std::visit([this](auto& app)
                    {
                    }, var);
                }
            }
        
        private:
            static constexpr size_t NumApps = sizeof...(TApps);
            std::array<AppVariant_t, NumApps> AppStorage;

            template <typename T>
            static AppVariant_t make_in_place_variant()
            {
                AppVariant_t var;
                var.template emplace<T>();  // construct in-place
                return var;
            }

            template <size_t... Idxs>
            static AppContainer init_storage(std::index_sequence<Idxs...>)
            {
                return {make_in_place_variant<TApps>()...};
            }
    };

} // namespace etfw

