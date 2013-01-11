// --------------------------------------------------------------------------
INTERFACE [arm && integrator]:

#include "kmem.h"

EXTENSION class Timer
{
public:
  static unsigned irq() { return 6; }

private:
  enum {
    Base = Kmem::Timer_map_base,

    TIMER0_VA_BASE = Base + 0x000,
    TIMER1_VA_BASE = Base + 0x100,
    TIMER2_VA_BASE = Base + 0x200,

    TIMER_LOAD   = 0x00,
    TIMER_VALUE  = 0x04,
    TIMER_CTRL   = 0x08,
    TIMER_INTCLR = 0x0c,

    TIMER_CTRL_IE       = 1 << 5,
    TIMER_CTRL_PERIODIC = 1 << 6,
    TIMER_CTRL_ENABLE   = 1 << 7,
  };
};

// ----------------------------------------------------------------------
IMPLEMENTATION [arm && integrator]:

#include "config.h"
#include "kip.h"
#include "io.h"

IMPLEMENT
void Timer::init(unsigned)
{
  /* Switch all timers off */
  Io::write(0, TIMER0_VA_BASE + TIMER_CTRL);
  Io::write(0, TIMER1_VA_BASE + TIMER_CTRL);
  Io::write(0, TIMER2_VA_BASE + TIMER_CTRL);

  unsigned timer_ctrl = TIMER_CTRL_ENABLE | TIMER_CTRL_PERIODIC;
  unsigned timer_reload = 1000000 / Config::Scheduler_granularity;

  Io::write(timer_reload, TIMER1_VA_BASE + TIMER_LOAD);
  Io::write(timer_reload, TIMER1_VA_BASE + TIMER_VALUE);
  Io::write(timer_ctrl | TIMER_CTRL_IE, TIMER1_VA_BASE + TIMER_CTRL);
}

static inline
Unsigned64
Timer::timer_to_us(Unsigned32 /*cr*/)
{ return 0; }

static inline
Unsigned64
Timer::us_to_timer(Unsigned64 us)
{ (void)us; return 0; }

PUBLIC static inline NEEDS["io.h"]
void
Timer::acknowledge()
{
  Io::write(1, TIMER1_VA_BASE + TIMER_INTCLR);
}

IMPLEMENT inline
void
Timer::update_one_shot(Unsigned64 wakeup)
{
  (void)wakeup;
}

IMPLEMENT inline NEEDS["kip.h"]
Unsigned64
Timer::system_clock()
{
  if (Config::Scheduler_one_shot)
    //return Kip::k()->clock + timer_to_us(Io::read<Unsigned32>(OSCR));
    return 0;
  else
    return Kip::k()->clock;
}

