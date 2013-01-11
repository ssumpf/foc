INTERFACE [arm]:

EXTENSION class Timer
{
public:
  static unsigned irq_mode() { return 0; }

private:
  static inline void update_one_shot(Unsigned64 wakeup);
};

// ------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "config.h"
#include "globals.h"
#include "kip.h"
#include "watchdog.h"

IMPLEMENT inline NEEDS["kip.h"]
void
Timer::init_system_clock()
{
  Kip::k()->clock = 0;
}

IMPLEMENT inline NEEDS["config.h", "globals.h", "kip.h", "watchdog.h"]
void
Timer::update_system_clock(unsigned cpu)
{
  if (cpu == 0)
    {
      Kip::k()->clock += Config::Scheduler_granularity;
      Watchdog::touch();
    }
}

IMPLEMENT inline NEEDS[Timer::update_one_shot, "config.h"]
void
Timer::update_timer(Unsigned64 wakeup)
{
  if (Config::Scheduler_one_shot)
    update_one_shot(wakeup);
}
