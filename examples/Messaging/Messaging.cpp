/**
 * @file Messaging.cpp
 * @author your name (you@domain.com)
 * @brief Example of app messaging
 * @version 0.1
 * @date 2025-09-05
 * 
 * @copyright Copyright (c) 2025
 * 
 * @details In this example, 
 * 
 */

#include "etfw/svcs/App.hpp"

#include "App1/ActiveApp1.hpp"
#include "App2/App2.hpp"

template <etfw::SvcId_t TAppId>
using ActiveAppCfg_t = etfw::SvcCfg<TAppId, etfw::ActiveSvcCfg<AppPriority, AppStackSz>>;

// ~~~~~~~~~~~~~~~ App 1 configuration ~~~~~~~~~~~~~~~

static constexpr etfw::SvcId_t App1Id = 1;

struct ActiveApp1Cfg : public ActiveAppCfg_t<App1Id>
{
    static constexpr const char* NAME = "EX_APP_1";
};

// ~~~~~~~~~~~~~~~ App 2 configuration ~~~~~~~~~~~~~~~

static constexpr etfw::SvcId_t App2Id = 2;

/// @brief Passive application configuration
struct ActiveApp2Cfg : public ActiveAppCfg_t<App2Id>
{
    static constexpr const char* NAME = "EX_APP_2";
};

// ~~~~~~~~~~~~~~~ App 3 configuration ~~~~~~~~~~~~~~~

static constexpr etfw::SvcId_t App3Id = 3;

/// @brief Passive application configuration
struct ActiveApp3Cfg : public ActiveAppCfg_t<App3Id>
{
    static constexpr const char* NAME = "EX_APP_3";
};


int main()
{
    app1::App app1;
    app2::App app2;
    printf("%s\n", app1.init().str());
    return 0;
}
