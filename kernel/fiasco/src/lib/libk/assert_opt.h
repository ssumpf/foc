#pragma once

#include <assert.h>
#include <fiasco_defs.h>

#if defined(NDEBUG) && FIASCO_GCC_VERSION >= 405
# define assert_opt(expr)  do { if (!(expr)) __builtin_unreachable(); } while (0)
#else
# define assert_opt(expr) assert(expr)
#endif



