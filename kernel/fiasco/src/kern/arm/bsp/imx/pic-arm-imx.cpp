// ---------------------------------------------------------------------
INTERFACE [arm && (imx21 || imx35)]:

#include "kmem.h"

class Irq_base;

EXTENSION class Pic
{
public:
  enum
  {
    INTCTL      = Kmem::Pic_map_base + 0x00,
    NIMASK      = Kmem::Pic_map_base + 0x04,
    INTENNUM    = Kmem::Pic_map_base + 0x08,
    INTDISNUM   = Kmem::Pic_map_base + 0x0c,
    INTENABLEH  = Kmem::Pic_map_base + 0x10,
    INTENABLEL  = Kmem::Pic_map_base + 0x14,
    INTTYPEH    = Kmem::Pic_map_base + 0x18,
    INTTYPEL    = Kmem::Pic_map_base + 0x1c,
    NIPRIORITY7 = Kmem::Pic_map_base + 0x20,
    NIPRIORITY0 = Kmem::Pic_map_base + 0x3c,
    NIVECSR     = Kmem::Pic_map_base + 0x40,
    FIVECSR     = Kmem::Pic_map_base + 0x44,
    INTSRCH     = Kmem::Pic_map_base + 0x48,
    INTSRCL     = Kmem::Pic_map_base + 0x4c,
    INTFRCH     = Kmem::Pic_map_base + 0x50,
    INTFRCL     = Kmem::Pic_map_base + 0x54,
    NIPNDH      = Kmem::Pic_map_base + 0x58,
    NIPNDL      = Kmem::Pic_map_base + 0x5c,
    FIPNDH      = Kmem::Pic_map_base + 0x60,
    FIPNDL      = Kmem::Pic_map_base + 0x64,


    INTCTL_FIAD  = 1 << 19, // Fast Interrupt Arbiter Rise ARM Level
    INTCTL_NIAD  = 1 << 20, // Normal Interrupt Arbiter Rise ARM Level
    INTCTL_FIDIS = 1 << 21, // Fast Interrupt Disable
    INTCTL_NIDIS = 1 << 22, // Normal Interrupt Disable
  };
};

// ---------------------------------------------------------------------
IMPLEMENTATION [arm && (imx21 || imx35)]:

#include "io.h"
#include "irq_chip_generic.h"
#include "irq_mgr.h"

class Irq_chip_arm_imx : public Irq_chip_gen
{
public:
  Irq_chip_arm_imx() : Irq_chip_gen(64) {}
  unsigned set_mode(Mword, unsigned) { return Irq_base::Trigger_level; }
  void set_cpu(Mword, unsigned) {}
  void ack(Mword) { /* ack is empty */ }
};

PUBLIC
void
Irq_chip_arm_imx::mask(Mword irq)
{
  assert(cpu_lock.test());
  Io::write<Mword>(irq, Pic::INTDISNUM); // disable pin
}

PUBLIC
void
Irq_chip_arm_imx::mask_and_ack(Mword irq)
{
  assert(cpu_lock.test());
  Io::write<Mword>(irq, Pic::INTDISNUM); // disable pin
  // ack is empty
}

PUBLIC
void
Irq_chip_arm_imx::unmask(Mword irq)
{
  assert (cpu_lock.test());
  Io::write<Mword>(irq, Pic::INTENNUM);
}

static Static_object<Irq_mgr_single_chip<Irq_chip_arm_imx> > mgr;


IMPLEMENT FIASCO_INIT
void Pic::init()
{
  Irq_mgr::mgr = mgr.construct();

  Io::write<Mword>(0,    INTCTL);
  Io::write<Mword>(0x10, NIMASK); // Do not disable any normal interrupts

  Io::write<Mword>(0, INTTYPEH); // All interrupts generate normal interrupts
  Io::write<Mword>(0, INTTYPEL);

  // Init interrupt priorities
  for (int i = 0; i < 8; ++i)
    Io::write<Mword>(0x1111, NIPRIORITY7 + (i * 4)); // low addresses start with 7
}

IMPLEMENT inline
Pic::Status Pic::disable_all_save()
{
  Status s = 0;
  return s;
}

IMPLEMENT inline
void Pic::restore_all(Status)
{}

PUBLIC static inline NEEDS["io.h"]
Unsigned32 Irq_chip_arm_imx::pending()
{
  return Io::read<Mword>(Pic::NIVECSR) >> 16;
}

PUBLIC inline NEEDS[Irq_chip_arm_imx::pending]
void
Irq_chip_arm_imx::irq_handler()
{
  Unsigned32 p = pending();
  if (EXPECT_TRUE(p != 0xffff))
    handle_irq<Irq_chip_arm_imx>(p, 0);
}

extern "C"
void irq_handler()
{ mgr->c.irq_handler(); }

//---------------------------------------------------------------------------
IMPLEMENTATION [debug && imx]:

PUBLIC
char const *
Irq_chip_arm_imx::chip_type() const
{ return "HW i.MX IRQ"; }
