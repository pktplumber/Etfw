
#pragma once

#include "etfw/svcs/App.hpp"

static constexpr uint8_t AppPriority = 19;
static constexpr size_t AppStackSz = 1024;

template <etfw::SvcId_t TAppId>
using ActiveAppCfg_t = etfw::SvcCfg<TAppId, etfw::ActiveSvcCfg<AppPriority, AppStackSz>>;
