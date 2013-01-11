IMPLEMENTATION[{ia32,amd64}-rtc_timer]:

#include "irq_chip.h"
#include "rtc.h"
#include "pit.h"

#include <cstdio>

//IMPLEMENT inline int Timer::irq() { return 8; }

PUBLIC static inline
unsigned Timer::irq() { return 8; }

PUBLIC static inline NEEDS["irq_chip.h"]
unsigned Timer::irq_mode()
{ return Irq_base::Trigger_edge | Irq_base::Polarity_high; }

IMPLEMENT
void
Timer::init(unsigned)
{
  printf("Using the RTC on IRQ %d (%sHz) for scheduling\n", 8,
#ifdef CONFIG_SLOW_RTC
         "64"
#else
         "1k"
#endif
      );

  // set up timer interrupt (~ 1ms)
  Rtc::init();

  // make sure that PIT does pull its interrupt line
  Pit::done();
}

PUBLIC static inline NEEDS["rtc.h"]
void
Timer::acknowledge()
{
  // periodic scheduling is triggered by irq 8 connected with RTC
  //  irq.mask();
  Rtc::reset();
  //  irq.ack();
  //  Rtc::reset();
  //  irq.unmask();
}

IMPLEMENT inline
void
Timer::update_timer(Unsigned64)
{
  // does nothing in periodic mode
}
