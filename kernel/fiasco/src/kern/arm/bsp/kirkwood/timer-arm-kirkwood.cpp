// --------------------------------------------------------------------------
INTERFACE [arm && kirkwood]:

#include "kmem.h"

EXTENSION class Timer
{
public:
  static unsigned irq() { return 1; }

private:
  enum {
    Control_Reg  = Mem_layout::Reset_map_base + 0x20300,
    Reload0_Reg  = Mem_layout::Reset_map_base + 0x20310,
    Timer0_Reg   = Mem_layout::Reset_map_base + 0x20314,
    Reload1_Reg  = Mem_layout::Reset_map_base + 0x20318,
    Timer1_Reg   = Mem_layout::Reset_map_base + 0x2031c,

    Bridge_cause = Mem_layout::Reset_map_base + 0x20110,
    Bridge_mask  = Mem_layout::Reset_map_base + 0x20114,

    Timer0_enable = 1 << 0,
    Timer0_auto   = 1 << 1,

    Timer0_bridge_num = 1 << 1,
    Timer1_bridge_num = 1 << 2,

    Reload_value = 200000,
  };
};

// ----------------------------------------------------------------------
IMPLEMENTATION [arm && kirkwood]:

#include "config.h"
#include "kip.h"
#include "io.h"

IMPLEMENT
void Timer::init(unsigned)
{
  // Disable timer
  Io::write(0, Control_Reg);

  // Set current timer value and reload value
  Io::write<Mword>(Reload_value, Timer0_Reg);
  Io::write<Mword>(Reload_value, Reload0_Reg);

  Io::set<Mword>(Timer0_enable | Timer0_auto, Control_Reg);

  Io::set<Unsigned32>(Timer0_bridge_num, Bridge_mask);
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
  Io::clear<Unsigned32>(Timer0_bridge_num, Bridge_cause);
}

IMPLEMENT inline
void
Timer::update_one_shot(Unsigned64 /*wakeup*/)
{
}

IMPLEMENT inline NEEDS["config.h", "kip.h"]
Unsigned64
Timer::system_clock()
{
  if (Config::Scheduler_one_shot)
    return 0;
  return Kip::k()->clock;
}
