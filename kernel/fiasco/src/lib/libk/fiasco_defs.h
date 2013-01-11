#pragma once

#define FIASCO_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

#if FIASCO_GCC_VERSION >= 403
# define FIASCO_COLD __attribute__((cold))
#else
# define FIASCO_COLD
#endif

#define FIASCO_PURE __attribute__((pure))

