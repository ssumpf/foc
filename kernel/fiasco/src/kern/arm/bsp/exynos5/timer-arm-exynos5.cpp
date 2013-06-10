INTERFACE [arm & exynos5]:

#include "kmem.h"
#include "processor.h"

EXTENSION class Timer
{
public:
  enum {
    BASE      = Kmem::Timer_map_base,
    CFG0      = BASE,
    CFG1      = BASE + 0x4,
    TCON      = BASE + 0x8,
    TCNTB0    = BASE + 0xc,
    TCMPB0    = BASE + 0x10,
    TINT_STAT = BASE + 0x44,
    ONE_MS    = 33000, /* HZ */
  };

    static unsigned irq() { return  68 + Proc::cpu_id(); }
};

IMPLEMENTATION [arm && exynos5]:

#include "cpu.h"
#include "io.h"
#include "irq_mgr.h"
#include "mmu.h"

IMPLEMENT inline
void
Timer::update_one_shot(Unsigned64 wakeup)
{
  (void)wakeup;
}

static inline
Mword
tcon_to_timer(Mword val, unsigned cpu_id)
{
  return  cpu_id == 0 ? val : (val << (4 + (4 * cpu_id)));
}

IMPLEMENT
void Timer::init(unsigned)
{
  unsigned cpu_id = Proc::cpu_id();

  if (Cpu::boot_cpu()->phys_id() == cpu_id)
    {
      // prescaler to one
      Io::write<Mword>(0x101, CFG0);
      // divider to 1
      Io::write<Mword>(0x0, CFG1);
    }
  // program 1ms
  Mword offset = 0xc * cpu_id;
  Io::write<Mword>(ONE_MS, TCNTB0 + offset);
  Io::write<Mword>(0x0, TCMPB0 + offset);

  // enable IRQ
  Io::set<Mword>(0x1 << cpu_id, TINT_STAT);

  // load and start timer in invterval mode
  Mword tcon = Io::read<Mword>(TCON);
  Io::write<Mword>(tcon | tcon_to_timer(0xa, cpu_id), TCON);
  Io::write<Mword>(tcon | tcon_to_timer(0x9, cpu_id), TCON);

  // route IRQ to this CPU
  Irq_mgr::mgr->set_cpu(irq(), cpu_id);
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

PUBLIC static inline NEEDS["io.h"]
void Timer::acknowledge()
{
  Mword stat = Io::read<Mword>(TINT_STAT);
  Io::write<Mword>(stat & (0x1f | (0x20 << Proc::cpu_id())), TINT_STAT);
}
