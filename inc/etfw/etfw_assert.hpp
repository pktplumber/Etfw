
#pragma once

#include <cassert>

#ifndef ETFW_NO_ASSERT_FAILURE
#define ETFW_ASSERT(check,err_msg)  assert((check) && err_msg)
#else
#define ETFW_ASSERT(check,err_msg)
#endif
