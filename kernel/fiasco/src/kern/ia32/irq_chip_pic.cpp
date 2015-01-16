INTERFACE:

#include "irq_chip_ia32.h"

/**
 * IRQ Chip based on the IA32 legacy PIC.
 *
 * Vectors for the PIC are from 0x20 to 0x2f statically assigned.
 */
class Irq_chip_ia32_pic : public Irq_chip_ia32
{
public:
  char const *chip_type() const { return "PIC"; }
};


IMPLEMENTATION:

#include <cassert>

#include "boot_alloc.h"
#include "cpu_lock.h"
#include "globalconfig.h"
#include "globals.h"
#include "irq_mgr.h"
#include "pic.h"

PUBLIC inline
Irq_chip_ia32_pic::Irq_chip_ia32_pic() : Irq_chip_ia32(16)
{}

PUBLIC
bool
Irq_chip_ia32_pic::alloc(Irq_base *irq, Mword irqn)
{
  // no mor than 16 IRQs
  if (irqn > 0xf)
    return false;

  // PIC uses vectors from 0x20 to 0x2f statically
  unsigned vector = 0x20 + irqn;

  return valloc(irq, irqn, vector);
}


PUBLIC
void
Irq_chip_ia32_pic::mask(Mword irq)
{
  Pic::disable_locked(irq);
}


PUBLIC
void
Irq_chip_ia32_pic::mask_and_ack(Mword irq)
{
  Pic::disable_locked(irq);
  Pic::acknowledge_locked(irq);
}

PUBLIC
void
Irq_chip_ia32_pic::ack(Mword irq)
{
  Pic::acknowledge_locked(irq);
}

PUBLIC
int
Irq_chip_ia32_pic::set_mode(Mword, Mode)
{ return 0; }

PUBLIC
bool
Irq_chip_ia32_pic::is_edge_triggered(Mword) const
{ return false; }

PUBLIC
void
Irq_chip_ia32_pic::unmask(Mword irq)
{
  Pic::enable_locked(irq, 0xa); //prio);
#if 0
  unsigned long prio;

  if (EXPECT_FALSE(!Irq::self(this)->owner()))
    return;
  if (Irq::self(this)->owner() == (Receiver*)-1)
    prio = ~0UL; // highes prio for JDB IRQs
  else
    prio = Irq::self(this)->owner()->sched()->prio();
#endif

}

PUBLIC
void
Irq_chip_ia32_pic::set_cpu(Mword, Cpu_number)
{}


class Pic_irq_mgr : public Irq_mgr
{
private:
  mutable Irq_chip_ia32_pic _pic;
};

PUBLIC Irq_mgr::Irq
Pic_irq_mgr::chip(Mword irq) const
{
  if (irq < 16)
    return Irq(&_pic, irq);

  return Irq();
}

PUBLIC
unsigned
Pic_irq_mgr::nr_irqs() const
{
  return 16;
}

PUBLIC
unsigned
Pic_irq_mgr::nr_msis() const
{ return 0; }


// ------------------------------------------------------------------------
IMPLEMENTATION [ux]:

PUBLIC static FIASCO_INIT
void
Irq_chip_ia32_pic::init()
{
  Irq_mgr::mgr = new Boot_object<Pic_irq_mgr>();
}

// ------------------------------------------------------------------------
IMPLEMENTATION [!ux]:

PUBLIC static FIASCO_INIT
void
Irq_chip_ia32_pic::init()
{
  Irq_mgr::mgr = new Boot_object<Pic_irq_mgr>();
  //
  // initialize interrupts
  //
  Irq_mgr::mgr->reserve(2);		// reserve cascade irq
  Irq_mgr::mgr->reserve(7);		// reserve spurious vect

  Pic::enable_locked(2);		// allow cascaded irqs
}
