IMPLEMENTATION:

#include "idt.h"
#include "irq_chip.h"
#include "irq_mgr.h"
#include "irq_chip_ia32.h"

#include "apic.h"
#include "static_init.h"
#include "boot_alloc.h"

class Irq_chip_msi : public Irq_chip_ia32
{
public:
  // this is somehow arbitrary
  enum { Max_msis = 0x40 };
  Irq_chip_msi() : Irq_chip_ia32(Max_msis) {}
};

class Irq_mgr_msi : public Irq_mgr
{
private:
  mutable Irq_chip_msi _chip;
  Irq_mgr *_orig;
};

PUBLIC inline
char const *
Irq_chip_msi::chip_type() const
{ return "MSI"; }

PUBLIC
bool
Irq_chip_msi::alloc(Irq_base *irq, Mword pin)
{
  return valloc(irq, pin, 0);
}

PUBLIC
void
Irq_chip_msi::unbind(Irq_base *irq)
{
  extern char entry_int_apic_ignore[];
  //Mword n = irq->pin();
  // hm: no way to mask an MSI: mask(n);
  vfree(irq, &entry_int_apic_ignore);
  Irq_chip_icu::unbind(irq);
}

PUBLIC
Mword
Irq_chip_msi::msg(Mword pin)
{
  if (pin < _irqs)
    return _entry[pin].vector();

  return 0;
}

PUBLIC int
Irq_chip_msi::set_mode(Mword, Mode)
{ return 0; }

PUBLIC bool
Irq_chip_msi::is_edge_triggered(Mword) const
{ return true; }

PUBLIC void
Irq_chip_msi::set_cpu(Mword, Cpu_number)
{}

PUBLIC void
Irq_chip_msi::mask(Mword)
{}

PUBLIC void
Irq_chip_msi::ack(Mword)
{ ::Apic::irq_ack(); }

PUBLIC void
Irq_chip_msi::mask_and_ack(Mword)
{ ::Apic::irq_ack(); }

PUBLIC void
Irq_chip_msi::unmask(Mword)
{}


PUBLIC inline explicit
Irq_mgr_msi::Irq_mgr_msi(Irq_mgr *o) : _orig(o) {}

PUBLIC Irq_mgr::Irq
Irq_mgr_msi::chip(Mword irq) const
{
  if (irq & 0x80000000)
    return Irq(&_chip, irq & ~0x80000000);
  else
    return _orig->chip(irq);
}

PUBLIC
unsigned
Irq_mgr_msi::nr_irqs() const
{ return _orig->nr_irqs(); }

PUBLIC
unsigned
Irq_mgr_msi::nr_msis() const
{ return _chip.nr_irqs(); }

PUBLIC
Mword
Irq_mgr_msi::msg(Mword irq) const
{
  if (irq & 0x80000000)
    return _chip.msg(irq & ~0x80000000);
  else
    return 0;
}

PUBLIC unsigned
Irq_mgr_msi::legacy_override(Mword irq)
{
  if (irq & 0x80000000)
    return irq;
  else
    return _orig->legacy_override(irq);
}


PUBLIC static FIASCO_INIT
void
Irq_mgr_msi::init()
{
  Irq_mgr_msi *m;
  Irq_mgr::mgr = m =  new Boot_object<Irq_mgr_msi>(Irq_mgr::mgr);
  printf("Enable MSI support: chained IRQ mgr @ %p\n", m->_orig);
}

STATIC_INITIALIZE(Irq_mgr_msi);

