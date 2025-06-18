
#pragma once

#include <type_traits>

namespace etfw
{
    // Helper to enforce base-class inheritance at compile time
    template <typename Base, typename... Ts>
    struct all_derived_from
    {
        static constexpr bool value = (std::is_base_of_v<Base, Ts> && ...);
    };
}
