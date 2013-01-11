// ---------------------------------------------------------------------
INTERFACE [arm-integrator]:

#include "kmem.h"

EXTENSION class Pic
{
public:
  enum
  {
    IRQ_STATUS       = Kmem::Pic_map_base + 0x00,
    IRQ_ENABLE_SET   = Kmem::Pic_map_base + 0x08,
    IRQ_ENABLE_CLEAR = Kmem::Pic_map_base + 0x0c,

    FIQ_ENABLE_CLEAR = Kmem::Pic_map_base + 0x2c,

    PIC_START = 0,
    PIC_END   = 31,
  };
};

// ---------------------------------------------------------------------
IMPLEMENTATION [arm && integrator]:

#include "io.h"
#include "irq_chip_generic.h"
#include "irq_mgr.h"

class Irq_chip_arm_integr : public Irq_chip_gen
{
public:
  Irq_chip_arm_integr() : Irq_chip_gen(32) {}
  unsigned set_mode(Mword, unsigned) { return Irq_base::Trigger_level; }
  void set_cpu(Mword, unsigned) {}
  void ack(Mword) { /* ack is empty */ }
};

PUBLIC
void
Irq_chip_arm_integr::mask(Mword irq)
{
  assert(cpu_lock.test());
  Io::write(1 << (irq - Pic::PIC_START), Pic::IRQ_ENABLE_CLEAR);
}

PUBLIC
void
Irq_chip_arm_integr::mask_and_ack(Mword irq)
{
  assert(cpu_lock.test());
  Io::write(1 << (irq - Pic::PIC_START), Pic::IRQ_ENABLE_CLEAR);
  // ack is empty
}

PUBLIC
void
Irq_chip_arm_integr::unmask(Mword irq)
{
  assert(cpu_lock.test());
  Io::write(1 << (irq - Pic::PIC_START), Pic::IRQ_ENABLE_SET);
}

static Static_object<Irq_mgr_single_chip<Irq_chip_arm_integr> > mgr;

IMPLEMENT FIASCO_INIT
void Pic::init()
{
  Irq_mgr::mgr = mgr.construct();

  Io::write(0xffffffff, IRQ_ENABLE_CLEAR);
  Io::write(0xffffffff, FIQ_ENABLE_CLEAR);
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
Unsigned32 Irq_chip_arm_integr::pending()
{
  return Io::read<Mword>(Pic::IRQ_STATUS);
}

extern "C"
void irq_handler()
{ mgr->c.handle_multi_pending<Irq_chip_arm_integr>(0); }

//---------------------------------------------------------------------------
IMPLEMENTATION [debug && integrator]:

PUBLIC
char const *
Irq_chip_arm_integr::chip_type() const
{ return "Integrator"; }
