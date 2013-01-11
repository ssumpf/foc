INTERFACE:

#include <types.h>
#include "initcalls.h"
#include <spin_lock.h>
#include "irq_chip_ia32.h"

class Acpi_madt;

class Io_apic_entry
{
  friend class Io_apic;
private:
  Unsigned64 _e;

public:
  enum Delivery { Fixed, Lowest_prio, SMI, NMI = 4, INIT, ExtINT = 7 };
  enum Dest_mode { Physical, Logical };
  enum Polarity { High_active, Low_active };
  enum Trigger { Edge, Level };

  Io_apic_entry() {}
  Io_apic_entry(Unsigned8 vector, Delivery d, Dest_mode dm, Polarity p,
                Trigger t, Unsigned8 dest)
    : _e(vector | (d << 8) | (dm << 11) | (p << 13) | (t << 15) | (1<<16)
         | (((Unsigned64)dest) << 56))
  {}

  unsigned delivery() const { return (_e >> 8) & 7; }
  void delivery(unsigned m) { _e = (_e & ~(7 << 8)) | ((m & 7) << 8); }
  unsigned dest_mode() const { return _e & (1 << 11); }
  void dest_mode(bool p) { _e = (_e & ~(1<<11)) | ((unsigned long)p << 11); }
  unsigned dest() const { return _e >> 56; }
  void dest(unsigned d) { _e = (_e & ~(0xffULL << 56)) | ((Unsigned64)d << 56); }
  unsigned mask() const { return _e & (1U << 16); }
  void mask(bool m) { _e = (_e & ~(1ULL << 16)) | ((Unsigned64)m << 16); }
  unsigned trigger() const { return _e & (1 << 15); }
  unsigned polarity() const { return _e & (1 << 13); }
  unsigned vector() const { return _e & 0xff; }
  void vector(Unsigned8 v) { _e = (_e & ~0xff) | v; }
  void trigger(Mword tr) { _e = (_e & ~(1UL << 15)) | ((tr & 1) << 15); }
  void polarity(Mword pl) { _e = (_e & ~(1UL << 13)) | ((pl & 1) << 13); }
} __attribute__((packed));


class Io_apic : public Irq_chip_ia32
{
  friend class Jdb_io_apic_module;

private:
  struct Apic
  {
    Unsigned32 volatile adr;
    Unsigned32 dummy[3];
    Unsigned32 volatile data;

    unsigned num_entries();
    Mword read(int reg);
    void modify(int reg, Mword set_bits, Mword del_bits);
    void write(int reg, Mword value);
  } __attribute__((packed));

  Apic *_apic;
  Spin_lock<> _l;
  unsigned _offset;
  Io_apic *_next;

  static unsigned _nr_irqs;
  static Io_apic *_first;
  static Acpi_madt const *_madt;
};

IMPLEMENTATION:

#include "acpi.h"
#include "apic.h"
#include "irq_mgr.h"
#include "kmem.h"
#include "kdb_ke.h"
#include "kip.h"
#include "lock_guard.h"
#include "boot_alloc.h"

Acpi_madt const *Io_apic::_madt;
unsigned Io_apic::_nr_irqs;
Io_apic *Io_apic::_first;


class Io_apic_mgr : public Irq_mgr
{
};

PUBLIC Irq_mgr::Irq
Io_apic_mgr::chip(Mword irq) const
{
  Io_apic *a = Io_apic::find_apic(irq);
  if (a)
    return Irq(a, irq - a->gsi_offset());

  return Irq(0, 0);
}

PUBLIC
unsigned
Io_apic_mgr::nr_irqs() const
{
  return Io_apic::total_irqs();
}

PUBLIC
unsigned
Io_apic_mgr::nr_msis() const
{ return 0; }

PUBLIC unsigned
Io_apic_mgr::legacy_override(Mword i)
{ return Io_apic::legacy_override(i); }


IMPLEMENT inline
Mword
Io_apic::Apic::read(int reg)
{
  adr = reg;
  asm volatile ("": : :"memory");
  return data;
}

IMPLEMENT inline
void
Io_apic::Apic::modify(int reg, Mword set_bits, Mword del_bits)
{
  register Mword tmp;
  adr = reg;
  asm volatile ("": : :"memory");
  tmp = data;
  tmp &= ~del_bits;
  tmp |= set_bits;
  data = tmp;
}

IMPLEMENT inline
void
Io_apic::Apic::write(int reg, Mword value)
{
  adr = reg;
  asm volatile ("": : :"memory");
  data = value;
}

IMPLEMENT inline
unsigned
Io_apic::Apic::num_entries()
{
  return (read(1) >> 16) & 0xff;
}

PUBLIC explicit inline
Io_apic::Io_apic(Io_apic::Apic *addr, unsigned irqs, unsigned gsi_base)
: Irq_chip_ia32(irqs), _apic(addr), _l(Spin_lock<>::Unlocked),
  _offset(gsi_base), _next(0)
{}


PUBLIC inline NEEDS["kdb_ke.h", "lock_guard.h"]
Io_apic_entry
Io_apic::read_entry(unsigned i)
{
  Lock_guard<decltype(_l)> g(&_l);
  Io_apic_entry e;
  //assert_kdb(i <= num_entries());
  e._e = (Unsigned64)_apic->read(0x10+2*i) | (((Unsigned64)_apic->read(0x11+2*i)) << 32);
  return e;
}


PUBLIC inline NEEDS["kdb_ke.h", "lock_guard.h"]
void
Io_apic::write_entry(unsigned i, Io_apic_entry const &e)
{
  Lock_guard<decltype(_l)> g(&_l);
  //assert_kdb(i <= num_entries());
  _apic->write(0x10+2*i, e._e);
  _apic->write(0x11+2*i, e._e >> 32);
}

PUBLIC static FIASCO_INIT
bool
Io_apic::init()
{
  _madt = Acpi::find<Acpi_madt const *>("APIC");

  if (_madt == 0)
    {
      printf("Could not find APIC in RSDT nor XSDT, skipping init\n");
      return false;
    }
  printf("IO-APIC: MADT = %p\n", _madt);

  int n_apics = 0;

  for (n_apics = 0;
       Acpi_madt::Io_apic const *ioapic = static_cast<Acpi_madt::Io_apic const *>(_madt->find(Acpi_madt::IOAPIC, n_apics));
       ++n_apics)
    {
      printf("IO-APIC[%2d]: struct: %p adr=%x\n", n_apics, ioapic, ioapic->adr);

      Address offs;
      Address va = Mem_layout::alloc_io_vmem(Config::PAGE_SIZE);
      assert (va);

      Kmem::map_phys_page(ioapic->adr, va, false, true, &offs);

      Kip::k()->add_mem_region(Mem_desc(ioapic->adr, ioapic->adr + Config::PAGE_SIZE -1, Mem_desc::Reserved));

      Io_apic::Apic *a = (Io_apic::Apic*)(va + offs);
      a->write(0, 0);

      unsigned const irqs = a->num_entries() + 1;
      Io_apic *apic = new Boot_object<Io_apic>(a, irqs, ioapic->irq_base);

      if ((apic->_offset + irqs) > _nr_irqs)
	_nr_irqs = apic->_offset + irqs;

      for (unsigned i = 0; i < irqs; ++i)
        {
          int v = 0x20+i;
          Io_apic_entry e(v, Io_apic_entry::Fixed, Io_apic_entry::Physical,
              Io_apic_entry::High_active, Io_apic_entry::Edge, 0);
          apic->write_entry(i, e);
        }

      Io_apic **c = &_first;
      while (*c && (*c)->_offset < apic->_offset)
	c = &((*c)->_next);

      apic->_next = *c;
      *c = apic;

      printf("IO-APIC[%2d]: pins %u\n", n_apics, irqs);
      apic->dump();
    }

  if (!n_apics)
    {
      printf("IO-APIC: Could not find IO-APIC in MADT, skip init\n");
      return false;
    }


  printf("IO-APIC: dual 8259: %s\n", _madt->apic_flags & 1 ? "yes" : "no");

  for (unsigned tmp = 0;;++tmp)
    {
      Acpi_madt::Irq_source const *irq
	= static_cast<Acpi_madt::Irq_source const *>(_madt->find(Acpi_madt::Irq_src_ovr, tmp));

      if (!irq)
	break;

      printf("IO-APIC: ovr[%2u] %02x -> %x\n", tmp, irq->src, irq->irq);
    }

  Irq_mgr::mgr = new Boot_object<Io_apic_mgr>();

  // in the case we use the IO-APIC not the PIC we can dynamically use
  // INT vectors from 0x20 to 0x2f too
  _vectors.add_free(0x20, 0x30);
  return true;
};

PUBLIC static
unsigned
Io_apic::total_irqs()
{ return _nr_irqs; }

PUBLIC static
unsigned
Io_apic::legacy_override(unsigned i)
{
  if (!_madt)
    return i;

  unsigned tmp = 0;
  for (;;++tmp)
    {
      Acpi_madt::Irq_source const *irq
	= static_cast<Acpi_madt::Irq_source const *>(_madt->find(Acpi_madt::Irq_src_ovr, tmp));

      if (!irq)
	break;

      if (irq->src == i)
	return irq->irq;
    }
  return i;
}

PUBLIC
void
Io_apic::dump()
{
  for (unsigned i = 0; i < _irqs; ++i)
    {
      Io_apic_entry e = read_entry(i);
      printf("  PIN[%2u%c]: vector=%2x, del=%u, dm=%s, dest=%u (%s, %s)\n",
	     i, e.mask() ? 'm' : '.',
	     e.vector(), e.delivery(), e.dest_mode() ? "logical" : "physical",
	     e.dest(),
	     e.polarity() ? "low" : "high",
	     e.trigger() ? "level" : "edge");
    }

}

PUBLIC inline
bool
Io_apic::valid() const { return _apic; }

PRIVATE inline NEEDS["kdb_ke.h", "lock_guard.h"]
void
Io_apic::_mask(unsigned irq)
{
  Lock_guard<decltype(_l)> g(&_l);
  //assert_kdb(irq <= _apic->num_entries());
  _apic->modify(0x10 + irq * 2, 1UL << 16, 0);
}

PRIVATE inline NEEDS["kdb_ke.h", "lock_guard.h"]
void
Io_apic::_unmask(unsigned irq)
{
  Lock_guard<decltype(_l)> g(&_l);
  //assert_kdb(irq <= _apic->num_entries());
  _apic->modify(0x10 + irq * 2, 0, 1UL << 16);
}

PUBLIC inline NEEDS["kdb_ke.h", "lock_guard.h"]
bool
Io_apic::masked(unsigned irq)
{
  Lock_guard<decltype(_l)> g(&_l);
  //assert_kdb(irq <= _apic->num_entries());
  return _apic->read(0x10 + irq * 2) & (1UL << 16);
}

PUBLIC inline
void
Io_apic::sync()
{
  (void)_apic->data;
}

PUBLIC inline NEEDS["kdb_ke.h", "lock_guard.h"]
void
Io_apic::set_dest(unsigned irq, Mword dst)
{
  Lock_guard<decltype(_l)> g(&_l);
  //assert_kdb(irq <= _apic->num_entries());
  _apic->modify(0x11 + irq * 2, dst & (~0UL << 24), ~0UL << 24);
}

PUBLIC inline
unsigned
Io_apic::gsi_offset() const { return _offset; }

PUBLIC static
Io_apic *
Io_apic::find_apic(unsigned irqnum)
{
  for (Io_apic *a = _first; a; a = a->_next)
    {
      if (a->_offset <= irqnum && a->_offset + a->_irqs > irqnum)
	return a;
    }
  return 0;
};

PUBLIC void
Io_apic::mask(Mword irq)
{
  _mask(irq);
  sync();
}

PUBLIC void
Io_apic::ack(Mword)
{
  ::Apic::irq_ack();
}

PUBLIC void
Io_apic::mask_and_ack(Mword irq)
{
  _mask(irq);
  sync();
  ::Apic::irq_ack();
}

PUBLIC void
Io_apic::unmask(Mword irq)
{
  _unmask(irq);
}

PUBLIC void
Io_apic::set_cpu(Mword irq, unsigned cpu)
{
  set_dest(irq, Cpu::cpus.cpu(cpu).phys_id());
}

static inline
Mword to_io_apic_trigger(unsigned mode)
{
  return (mode & Irq_base::Trigger_level)
            ? Io_apic_entry::Level
            : Io_apic_entry::Edge;
}

static inline
Mword to_io_apic_polarity(unsigned mode)
{
  return (mode & Irq_base::Polarity_low)
             ? Io_apic_entry::Low_active
             : Io_apic_entry::High_active;
}

PUBLIC unsigned
Io_apic::set_mode(Mword pin, unsigned mode)
{
  if ((mode & Irq_base::Polarity_mask) == Irq_base::Polarity_both)
    mode = Irq_base::Polarity_low;

  Io_apic_entry e = read_entry(pin);
  e.polarity(to_io_apic_polarity(mode));
  e.trigger(to_io_apic_trigger(mode));
  write_entry(pin, e);
  return mode;
}

PUBLIC
bool
Io_apic::alloc(Irq_base *irq, Mword pin)
{
  unsigned v = valloc(irq, pin, 0);

  if (!v)
    return false;

  Io_apic_entry e = read_entry(pin);
  e.vector(v);
  write_entry(pin, e);
  return true;
}

PUBLIC
void
Io_apic::unbind(Irq_base *irq)
{
  extern char entry_int_apic_ignore[];
  Mword n = irq->pin();
  mask(n);
  vfree(irq, &entry_int_apic_ignore);
  Irq_chip_icu::unbind(irq);
}

PUBLIC inline
char const *
Io_apic::chip_type() const
{ return "IO-APIC"; }

PUBLIC static inline
bool
Io_apic::active()
{ return _first; }
