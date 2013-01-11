INTERFACE [arm && s3c2410]:

#include "kmem.h"

EXTENSION class Timer
{
public:
  static unsigned irq() { return 14; }

private:
  enum {
    TCFG0  = Kmem::Timer_map_base + 0x00,
    TCFG1  = Kmem::Timer_map_base + 0x04,
    TCON   = Kmem::Timer_map_base + 0x08,
    TCNTB0 = Kmem::Timer_map_base + 0x0c,
    TCMPB0 = Kmem::Timer_map_base + 0x10,
    TCNTO0 = Kmem::Timer_map_base + 0x14,
    TCNTB1 = Kmem::Timer_map_base + 0x18,
    TCMPB1 = Kmem::Timer_map_base + 0x1c,
    TCNTO1 = Kmem::Timer_map_base + 0x20,
    TCNTB2 = Kmem::Timer_map_base + 0x24,
    TCMPB2 = Kmem::Timer_map_base + 0x28,
    TCNTO2 = Kmem::Timer_map_base + 0x2c,
    TCNTB3 = Kmem::Timer_map_base + 0x30,
    TCMPB3 = Kmem::Timer_map_base + 0x34,
    TCNTO3 = Kmem::Timer_map_base + 0x38,
    TCNTB4 = Kmem::Timer_map_base + 0x3c,
    TCNTO4 = Kmem::Timer_map_base + 0x40,

  };
};

// -----------------------------------------------------------------------
IMPLEMENTATION [arm && s3c2410]:

#include "config.h"
#include "kip.h"
#include "io.h"

#include <cstdio>

IMPLEMENT
void Timer::init(unsigned)
{
  Io::write(0, TCFG0); // prescaler config
  Io::write(0, TCFG1); // mux select
  Io::write(33333, TCNTB4); // reload value

  Io::write(5 << 20, TCON); // start + autoreload
}

PUBLIC static inline
void
Timer::acknowledge()
{}

static inline
Unsigned64
Timer::timer_to_us(Unsigned32 /*cr*/)
{ return 0; }

static inline
Unsigned64
Timer::us_to_timer(Unsigned64 us)
{ (void)us; return 0; }

IMPLEMENT inline
void
Timer::update_one_shot(Unsigned64 wakeup)
{
  (void)wakeup;
}

IMPLEMENT inline NEEDS["config.h", "kip.h", "io.h", Timer::timer_to_us]
Unsigned64
Timer::system_clock()
{
  if (Config::Scheduler_one_shot)
    //return Kip::k()->clock + timer_to_us(Io::read<Unsigned32>(OSCR));
    return 0;
  else
    return Kip::k()->clock;
}
