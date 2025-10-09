
#include <etfw/msg/Context.hpp>

// Definition for messaging context object
namespace etfw::msg
{
    GlobalContext glob;
    AppContext app_ctx;
    ChildContext child_ctx;
}

using namespace etfw::msg;

/// Global message broker. TODO: add configuration
Broker Context::broker_;

Context::Context()
{}
