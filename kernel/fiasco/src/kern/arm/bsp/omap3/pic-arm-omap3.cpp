INTERFACE [arm && omap3]:

#include "kmem.h"

class Irq_base;

EXTENSION class Pic
{
public:
  enum
  {
    INTCPS_SYSCONFIG         = Kmem::Intc_map_base + 0x010,
    INTCPS_SYSSTATUS         = Kmem::Intc_map_base + 0x014,
    INTCPS_CONTROL           = Kmem::Intc_map_base + 0x048,
    INTCPS_TRESHOLD          = Kmem::Intc_map_base + 0x068,
    INTCPS_ITRn_base         = Kmem::Intc_map_base + 0x080,
    INTCPS_MIRn_base         = Kmem::Intc_map_base + 0x084,
    INTCPS_MIR_CLEARn_base   = Kmem::Intc_map_base + 0x088,
    INTCPS_MIR_SETn_base     = Kmem::Intc_map_base + 0x08c,
    INTCPS_ISR_SETn_base     = Kmem::Intc_map_base + 0x090,
    INTCPS_ISR_CLEARn_base   = Kmem::Intc_map_base + 0x094,
    INTCPS_PENDING_IRQn_base = Kmem::Intc_map_base + 0x098,
    INTCPS_ILRm_base         = Kmem::Intc_map_base + 0x100,
  };
};

INTERFACE [arm && omap3_35x]: //-------------------------------------------

EXTENSION class Pic
{
public:
  enum { Num_irqs                 = 96, };
};

INTERFACE [arm && omap3_am33xx]: //----------------------------------------

EXTENSION class Pic
{
public:
  enum { Num_irqs                 = 128, };
};

//-------------------------------------------------------------------------
IMPLEMENTATION [arm && omap3]:

#include "config.h"
#include "io.h"
#include "irq_chip_generic.h"
#include "irq_mgr.h"

class Irq_chip_arm_omap3 : public Irq_chip_gen
{
public:
  Irq_chip_arm_omap3() : Irq_chip_gen(Pic::Num_irqs) {}
  unsigned set_mode(Mword, unsigned) { return Irq_base::Trigger_level; }
  void set_cpu(Mword, unsigned) {}
};

PUBLIC
void
Irq_chip_arm_omap3::mask(Mword irq)
{
  assert(cpu_lock.test());
  Io::write<Mword>(1 << (irq & 31), Pic::INTCPS_MIR_SETn_base + (irq & 0xe0));
}

PUBLIC
void
Irq_chip_arm_omap3::mask_and_ack(Mword irq)
{
  assert(cpu_lock.test());
  Io::write<Mword>(1 << (irq & 31), Pic::INTCPS_MIR_SETn_base + (irq & 0xe0));
  Io::write<Mword>(1, Pic::INTCPS_CONTROL);
}

PUBLIC
void
Irq_chip_arm_omap3::ack(Mword irq)
{
  (void)irq;
  Io::write<Mword>(1, Pic::INTCPS_CONTROL);
}

PUBLIC
void
Irq_chip_arm_omap3::unmask(Mword irq)
{
  assert(cpu_lock.test());
  Io::write<Mword>(1 << (irq & 31), Pic::INTCPS_MIR_CLEARn_base + (irq & 0xe0));
}

static Static_object<Irq_mgr_single_chip<Irq_chip_arm_omap3> > mgr;

IMPLEMENT FIASCO_INIT
void Pic::init()
{
  // Reset
  Io::write<Mword>(2, INTCPS_SYSCONFIG);
  while (!Io::read<Mword>(INTCPS_SYSSTATUS))
    ;

  // auto-idle
  Io::write<Mword>(1, INTCPS_SYSCONFIG);

  // disable treshold
  Io::write<Mword>(0xff, INTCPS_TRESHOLD);

  // set priority for each interrupt line, lets take 0x20
  // setting bit0 to 0 means IRQ (1 would mean FIQ)
  for (int m = 0; m < Num_irqs; ++m)
    Io::write<Mword>(0x20 << 2, INTCPS_ILRm_base + (4 * m));

  // mask all interrupts
  for (int n = 0; n < 3; ++n)
    Io::write<Mword>(0xffffffff, INTCPS_MIR_SETn_base + 0x20 * n);

  Irq_mgr::mgr = mgr.construct();
}

IMPLEMENT inline
Pic::Status Pic::disable_all_save()
{ return 0; }

IMPLEMENT inline
void Pic::restore_all(Status)
{}

PUBLIC static inline NEEDS["io.h"]
Unsigned32 Irq_chip_arm_omap3::pending()
{
  for (int n = 0; n < (Pic::Num_irqs >> 5); ++n)
    {
      unsigned long x = Io::read<Mword>(Pic::INTCPS_PENDING_IRQn_base + 0x20 * n);
      for (int i = 0; i < 32; ++i)
        if (x & (1 << i))
          return i + n * 32;
    }
  return 0;
}

extern "C"
void irq_handler()
{
  Unsigned32 i;
  while ((i = Irq_chip_arm_omap3::pending()))
    mgr->c.handle_irq<Irq_chip_arm_omap3>(i, 0);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [debug && omap3]:

PUBLIC
char const *
Irq_chip_arm_omap3::chip_type() const
{ return "HW OMAP3 IRQ"; }
