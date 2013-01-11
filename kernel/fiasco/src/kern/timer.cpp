INTERFACE:

#include "initcalls.h"
#include "l4_types.h"

class Timer
{
public:
  /**
   * Static constructor for the interval timer.
   *
   * The implementation is platform specific. Two x86 implementations
   * are timer-pit and timer-rtc.
   */
  static void init(unsigned cpu) FIASCO_INIT_CPU;

  /**
   * Initialize the system clock.
   */
  static void init_system_clock();

  /**
   * Advances the system clock.
   */
  static void update_system_clock(unsigned cpu);

  /**
   * Get the current system clock.
   */
  static Unsigned64 system_clock();

  /**
   * reprogram the one-shot timer to the next event.
   */
  static void update_timer(Unsigned64 wakeup);

  static void master_cpu(unsigned cpu) { _cpu = cpu; }

private:
  static unsigned _cpu;
};


IMPLEMENTATION:

unsigned Timer::_cpu;
