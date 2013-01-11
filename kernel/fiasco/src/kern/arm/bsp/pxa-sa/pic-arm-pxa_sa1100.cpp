INTERFACE [arm && pxa]: // -------------------------------------

#include "kmem.h"

EXTENSION class Pic
{
public:
  enum {
    ICIP = Kmem::Pic_map_base + 0x000000,
    ICMR = Kmem::Pic_map_base + 0x000004,
    ICLR = Kmem::Pic_map_base + 0x000008,
    ICCR = Kmem::Pic_map_base + 0x000014,
    ICFP = Kmem::Pic_map_base + 0x00000c,
    ICPR = Kmem::Pic_map_base + 0x000010,
  };
};

INTERFACE [arm && sa1100]: // ----------------------------------

#include "kmem.h"

EXTENSION class Pic
{
public:
  enum {
    ICIP = Kmem::Pic_map_base + 0x00000,
    ICMR = Kmem::Pic_map_base + 0x00004,
    ICLR = Kmem::Pic_map_base + 0x00008,
    ICCR = Kmem::Pic_map_base + 0x0000c,
    ICFP = Kmem::Pic_map_base + 0x00010,
    ICPR = Kmem::Pic_map_base + 0x00020,
  };
};

// -------------------------------------------------------------
IMPLEMENTATION [arm && (sa1100 || pxa)]:

#include "config.h"
#include "io.h"
#include "irq.h"
#include "irq_chip_generic.h"
#include "irq_mgr.h"

class Chip : public Irq_chip_gen
{
public:
  Chip() : Irq_chip_gen(32) {}
  unsigned set_mode(Mword, unsigned) { return Irq_base::Trigger_level; }
  void set_cpu(Mword, unsigned) {}
  void ack(Mword) { /* ack is empty */ }
};

PUBLIC
void
Chip::mask(Mword irq)
{
  assert(cpu_lock.test());
  Io::write(Io::read<Mword>(Pic::ICMR) & ~(1 << irq), Pic::ICMR);
}

PUBLIC
void
Chip::mask_and_ack(Mword irq)
{
  assert (cpu_lock.test());
  Io::write(Io::read<Mword>(Pic::ICMR) & ~(1 << irq), Pic::ICMR);
  // ack is empty
}

PUBLIC
void
Chip::unmask(Mword irq)
{
  Io::write(Io::read<Mword>(Pic::ICMR) | (1 << irq), Pic::ICMR);
}

static Static_object<Irq_mgr_single_chip<Chip> > mgr;

IMPLEMENT FIASCO_INIT
void Pic::init()
{
  Irq_mgr::mgr = mgr.construct();

  // only unmasked interrupts wakeup from idle
  Io::write(0x01, ICCR);
  // mask all interrupts
  Io::write(0x00, ICMR);
  // all interrupts are IRQ's (no FIQ)
  Io::write(0x00, ICLR);
}

IMPLEMENT inline NEEDS["io.h"]
Pic::Status Pic::disable_all_save()
{
  Status s = Io::read<Mword>(ICMR);
  Io::write(0, ICMR);
  return s;
}

IMPLEMENT inline NEEDS["io.h"]
void Pic::restore_all(Status s)
{
  Io::write(s, ICMR);
}

PUBLIC static inline NEEDS["io.h"]
Unsigned32 Chip::pending()
{
  return Io::read<Unsigned32>(Pic::ICIP);
}

extern "C"
void irq_handler()
{
  Unsigned32 i = Chip::pending();
  if (i)
    mgr->c.handle_irq<Chip>(i, 0);
}

// -------------------------------------------------------------
IMPLEMENTATION [arm && debug && (sa1100 || pxa)]:

PUBLIC
char const *
Chip::chip_type() const
{ return "HW PXA/SA IRQ"; }
