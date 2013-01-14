INTERFACE:

#include "irq_chip.h"

class Timer_tick : public Irq_base
{
public:
  enum Mode
  {
    Any_cpu, ///< Might hit on any CPU
    Sys_cpu, ///< Hit only on the CPU that manages the system time
    App_cpu, ///< Hit only on application CPUs
  };
  /// Create a timer IRQ object
  explicit Timer_tick(Mode mode)
  {
    switch (mode)
      {
      case Any_cpu: set_hit(&handler_all); break;
      case Sys_cpu: set_hit(&handler_sys_time); break;
      case App_cpu: set_hit(&handler_app); break;
      }
  }

  static void setup(unsigned cpu);
  static void enable(unsigned cpu);
  static void disable(unsigned cpu);

protected:
  static bool allocate_irq(Irq_base *irq, unsigned irqnum);

private:
  // we do not support triggering modes
  void switch_mode(unsigned) {}
};

// ------------------------------------------------------------------------
INTERFACE [debug]:

#include "tb_entry.h"

EXTENSION class Timer_tick
{
public:
  struct Log : public Tb_entry
  {
    Irq_base *obj;
    Address user_ip;
    unsigned print(int, char *) const;
  };
};

// ------------------------------------------------------------------------
IMPLEMENTATION:

#include "thread.h"
#include "timer.h"

PUBLIC static inline NEEDS["thread.h", "timer.h"]
void
Timer_tick::handler_all(Irq_base *_s, Upstream_irq const *ui)
{
  Timer_tick *self = nonull_static_cast<Timer_tick *>(_s);
  self->ack();
  ui->ack();
  Thread *t = current_thread();
  Timer::update_system_clock(t->cpu(true));
  self->log_timer();
  t->handle_timer_interrupt();
}

PUBLIC static inline NEEDS["thread.h", "timer.h"]
void
Timer_tick::handler_sys_time(Irq_base *_s, Upstream_irq const *ui)
{
  Timer_tick *self = nonull_static_cast<Timer_tick *>(_s);
  self->ack();
  ui->ack();
  Timer::update_system_clock(0); // assume 0 to be the CPU that
                                 // manages the system time
  self->log_timer();
  current_thread()->handle_timer_interrupt();
}

PUBLIC static inline NEEDS["thread.h", "timer.h"]
void
Timer_tick::handler_app(Irq_base *_s, Upstream_irq const *ui)
{
  Timer_tick *self = nonull_static_cast<Timer_tick *>(_s);
  self->ack();
  ui->ack();
  self->log_timer();
  current_thread()->handle_timer_interrupt();
}

// --------------------------------------------------------------------------
IMPLEMENTATION [!debug]:

PUBLIC static inline
void
Timer_tick::log_timer()
{}

// --------------------------------------------------------------------------
IMPLEMENTATION [debug]:

#include "logdefs.h"
#include "irq_chip.h"

IMPLEMENT
unsigned
Timer_tick::Log::print(int maxlen, char *buf) const
{
  return snprintf(buf, maxlen, "u-ip=0x%lx", user_ip);
}

PUBLIC inline NEEDS["logdefs.h"]
void
Timer_tick::log_timer()
{
  Context *c = current();
  LOG_TRACE("Timer IRQs (kernel scheduling)", "timer", c, Log,
      l->user_ip  = c->regs()->ip();
      l->obj      = this;
  );
}
