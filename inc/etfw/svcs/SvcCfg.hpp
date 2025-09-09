
#pragma once

#include "SvcTypes.hpp"
#include "Runner.hpp"
#include "msg/Router.hpp"
#include <string>
#include "CommonTraits.hpp"

namespace etfw {

    class iSvc;

    /// @brief Type list helper struct
    /// @tparam ...Ts Types
    template <typename... Ts>
    struct TypeList
    {
        template <template <typename...> class Target>
        using ExpandTo = Target<Ts...>;
    };

    // Trait to compute max size and align
    template<typename... Ts>
    struct MaxSizeAlign {
        static constexpr std::size_t size = std::max({sizeof(Ts)...});
        static constexpr std::size_t align = std::max({alignof(Ts)...});
    };

    /// @brief Base service run trait. Used to enforce trait compliance.
    struct SvcRunTrait {};

    /// @brief Passive service trait
    struct PassiveSvcCfg : public SvcRunTrait
    {
        using Runner_t = etfw::PassiveRunner;

        template <typename TConcreteSvc, typename... TMsgs>
        using MsgHandler_t = etfw::msg::Router<TConcreteSvc, 0, TMsgs...>;

        template <typename TConcreteSvc, typename... TMsgs>
        using CmdHandler_t = MsgHandler_t<TConcreteSvc, TMsgs...>;
    };

    template <uint8_t TPriority, size_t TStackSz>
    struct ActiveSvcCfg : public SvcRunTrait
    {
        static constexpr uint8_t PRIORITY = TPriority;
        static constexpr size_t STACK_SZ = TStackSz;
        using Runner_t = etfw::ActiveRunner<TPriority, TStackSz>;
    };

    //template <etfw::SvcId_t TId, typename TRunnerTrait>
    //struct SvcCfg;

    template <etfw::SvcId_t TId, typename TRunnerTrait>
    struct SvcCfg
    {
        //static_assert(
        //    std::is_base_of<SvcRunTrait, PassiveSvcCfg>::value ||
        //    std::is_base_of<SvcRunTrait, ActiveSvcCfg>::value,
        //    "RunnerTrait must derive from PassiveSvcCfg or ActiveSvcCfg");

        //private:
        //    template<typename T, size_t I>
        //    static iSvc* construct()
        //    {
        //        return new (ChildrenBuf[I]) T();
        //    }

        //public:
            //using ChildrenStorageTraits = MaxSizeAlign<TChildren...>;
            //static constexpr size_t MaxChildSz = ChildrenStorageTraits::size;
            //static constexpr size_t ChildAlign = ChildrenStorageTraits::align;

            //alignas(ChildAlign) static inline uint8_t ChildrenBuf[sizeof...(TChildren)][MaxChildSz];

            //static inline iSvc* children[sizeof...(TChildren)] =
            //{
            //    construct<TChildren, index>()...
            //};

            /// @brief Service ID configuration
            static constexpr etfw::SvcId_t ID = TId;

            /// @brief Runner type
            using RunnerCfg = TRunnerTrait;
            //using ChildrenTs = TypeList<TChildren...>;
    };

    template <uint8_t TPriority, size_t TStackSz>
    struct ChildSvcCfg
    {
        using Runner_t = etfw::ActiveRunner<TPriority, TStackSz>;
    };

    template<typename T, typename = void>
    struct valid_svc_name : std::false_type {};

    /// @brief Trait to detect static constexpr const char* NAME. Used in svc config
    /// @tparam T Type input
    template<typename T>
    struct valid_svc_name<T, std::void_t<decltype(T::NAME)>> 
        : std::is_same<decltype(T::NAME), SvcNameRaw_t> {};

}
