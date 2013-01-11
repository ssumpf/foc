
INTERFACE:

#include "timeout.h"

class Timeslice_timeout : public Timeout
{
};

IMPLEMENTATION:

#include <cassert>
#include "globals.h"
#include "sched_context.h"
#include "std_macros.h"

/* Initialize global valiable timeslice_timeout */
DEFINE_PER_CPU Per_cpu<Timeout *> timeslice_timeout;
DEFINE_PER_CPU static Per_cpu<Timeslice_timeout> the_timeslice_timeout(true);

PUBLIC
Timeslice_timeout::Timeslice_timeout(unsigned cpu)
{
  timeslice_timeout.cpu(cpu) = this;
}


/**
 * Timeout expiration callback function
 * @return true to force a reschedule
 */
PRIVATE
bool
Timeslice_timeout::expired()
{
  unsigned cpu = current_cpu();
  Sched_context *sched = Sched_context::rq(cpu).current_sched();

  if (sched)
    {
#if 0
      Context *owner = sched->owner();

      // Ensure sched is owner's current timeslice
      assert (owner->sched() == sched);
#endif
      sched->replenish();
      sched->requeue(cpu);
      sched->invalidate_sched(cpu);

//      owner->switch_sched(sched);
    }

  return true;				// Force reschedule
}
