INTERFACE [exynos5]:
#include "panic.h"

#define NOT_IMPL WARN "%s not implemented", __PRETTY_FUNCTION__
#define NOT_IMPL_PANIC panic("%s not implemented (from %p)\n", __PRETTY_FUNCTION__, __builtin_return_address((0)));
