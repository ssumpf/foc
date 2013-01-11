// --------------------------------------------------------------------------
INTERFACE [arm]:

#include "kmem.h"

class Timer_sp804
{
public:
  enum {
    System_control = Kmem::System_ctrl_map_base,

    Refclk = 0,
    Timclk = 1,

    Timer0_enable = 15,
    Timer1_enable = 17,
    Timer2_enable = 19,
    Timer3_enable = 21,

    Timer_load   = 0x00,
    Timer_value  = 0x04,
    Timer_ctrl   = 0x08,
    Timer_intclr = 0x0c,

    Load_0 = Kmem::Timer0_map_base + Timer_load,
    Load_1 = Kmem::Timer1_map_base + Timer_load,
    Load_2 = Kmem::Timer2_map_base + Timer_load,
    Load_3 = Kmem::Timer3_map_base + Timer_load,

    Value_0 = Kmem::Timer0_map_base + Timer_value,
    Value_1 = Kmem::Timer1_map_base + Timer_value,
    Value_2 = Kmem::Timer2_map_base + Timer_value,
    Value_3 = Kmem::Timer3_map_base + Timer_value,

    Ctrl_0 = Kmem::Timer0_map_base + Timer_ctrl,
    Ctrl_1 = Kmem::Timer1_map_base + Timer_ctrl,
    Ctrl_2 = Kmem::Timer2_map_base + Timer_ctrl,
    Ctrl_3 = Kmem::Timer3_map_base + Timer_ctrl,

    Intclr_0 = Kmem::Timer0_map_base + Timer_intclr,
    Intclr_1 = Kmem::Timer1_map_base + Timer_intclr,
    Intclr_2 = Kmem::Timer2_map_base + Timer_intclr,
    Intclr_3 = Kmem::Timer3_map_base + Timer_intclr,

    Interval = 1000,

    Ctrl_ie        = 1 << 5,
    Ctrl_periodic  = 1 << 6,
    Ctrl_enable    = 1 << 7,
  };
};

// --------------------------------------------------------------------------
INTERFACE [arm && sp804]:

EXTENSION class Timer
{
public:
  static unsigned irq() { return 36; }

private:
  enum {
    Interval = 1000,
  };
};

// -----------------------------------------------------------------------
IMPLEMENTATION [arm && sp804]:

#include "config.h"
#include "kip.h"
#include "io.h"

#include <cstdio>

IMPLEMENT
void Timer::init(unsigned)
{
  Mword v = Io::read<Mword>(Timer_sp804::System_control);
  v |= Timer_sp804::Timclk << Timer_sp804::Timer0_enable;
  Io::write<Mword>(v, Timer_sp804::System_control);

  // all timers off
  Io::write<Mword>(0, Timer_sp804::Ctrl_0);
  Io::write<Mword>(0, Timer_sp804::Ctrl_1);
  Io::write<Mword>(0, Timer_sp804::Ctrl_2);
  Io::write<Mword>(0, Timer_sp804::Ctrl_3);

  Io::write<Mword>(Interval, Timer_sp804::Load_0);
  Io::write<Mword>(Interval, Timer_sp804::Value_0);
  Io::write<Mword>  (Timer_sp804::Ctrl_enable
                   | Timer_sp804::Ctrl_periodic
                   | Timer_sp804::Ctrl_ie,
                   Timer_sp804::Ctrl_0);
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
  Io::write<Mword>(0, Timer_sp804::Intclr_0);
}

IMPLEMENT inline
void
Timer::update_one_shot(Unsigned64 wakeup)
{
  (void)wakeup;
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
