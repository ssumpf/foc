INTERFACE:

#include "types.h"
#include "per_cpu_data.h"

EXTENSION class Timer_tick
{
public:
  static Per_cpu<Timer_tick> _glbl_timer;

  Timer_tick()
  {
   if (Proc::cpu_id())
     set_hit(&handler_app);
   else
     set_hit(&handler_sys_time);
  }
};

IMPLEMENTATION:

#include "timer.h"

DEFINE_PER_CPU Per_cpu<Timer_tick> Timer_tick::_glbl_timer;

IMPLEMENT void
Timer_tick::setup(unsigned cpu)
{
  if (!allocate_irq(&_glbl_timer.cpu(cpu), Timer::irq()))
    panic("Could not allocate scheduling IRQ %d\n", Timer::irq());

  _glbl_timer.cpu(cpu).set_mode(Timer::irq_mode());
}

IMPLEMENT
void
Timer_tick::enable(unsigned)
{
  _glbl_timer.current().chip()->unmask(_glbl_timer.current().pin());
}

IMPLEMENT
void
Timer_tick::disable(unsigned)
{
  _glbl_timer.current().chip()->mask(_glbl_timer.current().pin());
}

PUBLIC inline NEEDS["timer.h"]
void
Timer_tick::ack()
{
  Timer::acknowledge();
  Irq_base::ack();
}
