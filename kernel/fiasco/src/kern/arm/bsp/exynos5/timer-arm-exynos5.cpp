INTERFACE [arm & exynos5]:

#include "kmem.h"

EXTENSION class Timer
{
public:
  enum {
    BASE = Kmem::Timer_map_base,
    CFG0      = BASE,
    CFG1      = BASE + 0x4,
    TCON      = BASE + 0x8,
    TCNTB0    = BASE + 0xc,
    TCMPB0    = BASE + 0x10,
    TINT_STAT = BASE + 0x44,
    ONE_MS    = 33000, /* HZ */
  };

    static unsigned irq() { return  68; /* timer0 */ }
};

IMPLEMENTATION [arm && exynos5]:

#include "mmu.h"
#include "io.h"

IMPLEMENT inline
void
Timer::update_one_shot(Unsigned64 wakeup)
{
  (void)wakeup;
}


IMPLEMENT
void Timer::init(unsigned)
{
  /* prescaler to one */
  Io::write<Mword>(0x1, CFG0);
  /* divider to 1 */
  Io::write<Mword>(0x0, CFG1);

  /* program 1ms */
  Io::write<Mword>(ONE_MS, TCNTB0);
  Io::write<Mword>(0x0, TCMPB0);

  /* enable IRQ */
  Io::write<Mword>(0x1, TINT_STAT);

  /* load and start timer in invterval mode*/
  Io::write<Mword>(0xa, TCON);
  Io::write<Mword>(0x9, TCON);
}

IMPLEMENT inline NEEDS["config.h", "kip.h"]
Unsigned64
Timer::system_clock()
{
  if (Config::Scheduler_one_shot)
    return 0;
  else
    return Kip::k()->clock;
}

PUBLIC static inline NEEDS["io.h"]
void Timer::acknowledge()
{
  Io::set<Mword>(0x20, TINT_STAT);
}
