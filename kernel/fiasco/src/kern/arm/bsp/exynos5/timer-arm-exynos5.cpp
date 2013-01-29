INTERFACE [arm & exynos5]:

EXTENSION class Timer
{
  public:
    static unsigned irq() { return  64; /* timer0 */ }
};

IMPLEMENTATION [arm && exynos5]:

#include "warn.h"

IMPLEMENT inline
void
Timer::update_one_shot(Unsigned64 wakeup)
{
  (void)wakeup;
}


IMPLEMENT
void Timer::init(unsigned)
{
//	NOT_IMPL_PANIC;
}

IMPLEMENT inline NEEDS["warn.h"]
Unsigned64
Timer::system_clock()
{
	NOT_IMPL_PANIC;
  return 0;
}

PUBLIC static inline NEEDS["warn.h"]
void Timer::acknowledge()
{
	NOT_IMPL_PANIC;
}
