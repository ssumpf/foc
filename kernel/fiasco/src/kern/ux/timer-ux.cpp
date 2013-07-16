// ------------------------------------------------------------------------
INTERFACE[ux]:

#include "irq_chip.h"

EXTENSION class Timer
{
private:
  static void bootstrap();
};

// ------------------------------------------------------------------------
IMPLEMENTATION[ux]:

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include "boot_info.h"
#include "initcalls.h"
#include "irq_mgr.h"
#include "pic.h"

PUBLIC static inline NEEDS["pic.h"]
unsigned
Timer::irq() { return Pic::Irq_timer; }

PUBLIC static inline NEEDS["irq_chip.h"]
Irq_chip::Mode
Timer::irq_mode()
{ return Irq_chip::Mode::F_raising_edge; }

IMPLEMENT FIASCO_INIT_CPU
void
Timer::init(Cpu_number)
{
  if (Boot_info::irq0_disabled())
    return;

  if (!Pic::setup_irq_prov(Pic::Irq_timer, Boot_info::irq0_path(), bootstrap))
    {
      puts ("Problems setting up timer interrupt!");
      exit (1);
    }
}

IMPLEMENT FIASCO_INIT_CPU
void
Timer::bootstrap()
{
  close(Boot_info::fd());
  execl(Boot_info::irq0_path(), "[I](irq0)", NULL);
}

PUBLIC static inline
void
Timer::acknowledge()
{}


IMPLEMENT inline
void
Timer::update_timer(Unsigned64)
{
  // does nothing in periodic mode
}
