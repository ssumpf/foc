// --------------------------------------------------------------------------
INTERFACE [arm && mptimer]:

#include "irq_chip.h"
#include "kmem.h"

EXTENSION class Timer
{
public:
  static unsigned irq() { return 29; }

private:
  enum
  {
    Timer_load_reg     = Kmem::Mp_scu_map_base + 0x600 + 0x0,
    Timer_counter_reg  = Kmem::Mp_scu_map_base + 0x600 + 0x4,
    Timer_control_reg  = Kmem::Mp_scu_map_base + 0x600 + 0x8,
    Timer_int_stat_reg = Kmem::Mp_scu_map_base + 0x600 + 0xc,

    Prescaler = 0,

    Timer_control_enable    = 1 << 0,
    Timer_control_reload    = 1 << 1,
    Timer_control_itenable  = 1 << 2,
    Timer_control_prescaler = (Prescaler & 0xff) << 8,

    Timer_int_stat_event   = 1,
  };
};

// --------------------------------------------------------------
IMPLEMENTATION [arm && mptimer]:

#include <cstdio>
#include "config.h"
#include "io.h"
#include "irq_chip.h"
#include "kip.h"

#include "globals.h"

PRIVATE static
Mword
Timer::start_as_counter()
{
  Io::write<Mword>(Timer_control_prescaler | Timer_control_reload
                   | Timer_control_enable,
                   Timer_control_reg);

  Mword v = ~0UL;
  Io::write<Mword>(v, Timer_counter_reg);
  return v;
}

PRIVATE static
Mword
Timer::stop_counter()
{
  Mword v = Io::read<Mword>(Timer_counter_reg);
  Io::write<Mword>(0, Timer_control_reg);
  return v;
}

IMPLEMENT
void
Timer::init(unsigned)
{
  Mword i = interval();

  Io::write<Mword>(i, Timer_load_reg);
  Io::write<Mword>(i, Timer_counter_reg);
  Io::write<Mword>(Timer_control_prescaler | Timer_control_reload
                   | Timer_control_enable | Timer_control_itenable,
                   Timer_control_reg);
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
  Io::write<Mword>(Timer_int_stat_event, Timer_int_stat_reg);
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
  return Kip::k()->clock;
}
