INTERFACE [arm && pic_gic]:

#include "kmem.h"
#include "irq_chip_generic.h"


class Gic : public Irq_chip_gen
{
private:
  Address _cpu_base;
  Address _dist_base;

public:
  enum
  {
    DIST_CTRL         = 0x000,
    DIST_CTR          = 0x004,
    DIST_IRQ_SEC      = 0x080,
    GICD_IGROUPR      = DIST_IRQ_SEC, // new name
    DIST_ENABLE_SET   = 0x100,
    DIST_ENABLE_CLEAR = 0x180,
    DIST_SET_PENDING  = 0x200,
    DIST_CLR_PENDING  = 0x280,
    DIST_PRI          = 0x400,
    DIST_TARGET       = 0x800,
    DIST_CONFIG       = 0xc00,
    DIST_SOFTINT      = 0xf00,

    MXC_TZIC_PRIOMASK = 0x00c,
    MXC_TZIC_SYNCCTRL = 0x010,
    MXC_TZIC_PND      = 0xd00,

    CPU_CTRL          = 0x00,
    CPU_PRIMASK       = 0x04,
    CPU_BPR           = 0x08,
    CPU_INTACK        = 0x0c,
    CPU_EOI           = 0x10,
    CPU_RUNINT        = 0x14,
    CPU_PENDING       = 0x18,

    DIST_CTRL_ENABLE         = 1,

    MXC_TZIC_CTRL_NSEN       = 1 << 16,
    MXC_TZIC_CTRL_NSENMASK   = 1 << 31,

    CPU_CTRL_ENABLE_GRP0     = 1 << 0,
    CPU_CTRL_ENABLE          = CPU_CTRL_ENABLE_GRP0,
    CPU_CTRL_ENABLE_GRP1     = 1 << 1,
    CPU_CTRL_USE_FIQ_FOR_SEC = 1 << 3,
  };

};

// ------------------------------------------------------------------------
INTERFACE [arm && pic_gic && pic_gic_mxc_tzic]:

EXTENSION class Gic { enum { Config_mxc_tzic = 1 }; };

// ------------------------------------------------------------------------
INTERFACE [arm && pic_gic && !pic_gic_mxc_tzic]:

EXTENSION class Gic { enum { Config_mxc_tzic = 0 }; };

// ------------------------------------------------------------------------
INTERFACE [arm && arm_em_tz]:

EXTENSION class Gic { enum { Config_tz_sec = 1 }; };

// ------------------------------------------------------------------------
INTERFACE [arm && !arm_em_tz]:

EXTENSION class Gic { enum { Config_tz_sec = 0 }; };


//-------------------------------------------------------------------
IMPLEMENTATION [arm && pic_gic]:

#include <cassert>
#include <cstring>
#include <cstdio>

#include "cascade_irq.h"
#include "io.h"
#include "irq_chip_generic.h"
#include "panic.h"
#include "processor.h"

PUBLIC inline NEEDS["io.h"]
unsigned
Gic::hw_nr_irqs()
{ return ((Io::read<Mword>(_dist_base + DIST_CTR) & 0x1f) + 1) * 32; }

PUBLIC inline NEEDS["io.h"]
bool
Gic::has_sec_ext()
{ return Io::read<Mword>(_dist_base + DIST_CTR) & (1 << 10); }

PUBLIC inline
void Gic::softint_cpu(unsigned callmap, unsigned m)
{
  Io::write<Mword>(((callmap & 0xff) << 16) | m, _dist_base + DIST_SOFTINT);
}

PUBLIC inline
void Gic::softint_bcast(unsigned m)
{ Io::write<Mword>((1 << 24) | m, _dist_base + DIST_SOFTINT); }

PRIVATE
void
Gic::cpu_init()
{
  Io::write<Mword>(0, _cpu_base + CPU_CTRL);

  Io::write<Mword>(0xffffffff, _dist_base + DIST_ENABLE_CLEAR);
  if (Config_tz_sec)
    {
      Io::write<Mword>(0x00000f00, _dist_base + DIST_ENABLE_SET);
      Io::write<Mword>(0xfffff0ff, _dist_base + GICD_IGROUPR);
    }
  else
    {
      Io::write<Mword>(0x0000001e, _dist_base + DIST_ENABLE_SET);
      Io::write<Mword>(0, _dist_base + GICD_IGROUPR);
    }

  Io::write<Mword>(0xffffffff, _dist_base + DIST_CLR_PENDING);

  Io::write<Mword>(0xffffffff, _dist_base + 0x380); // clear active
  Io::write<Mword>(0xffffffff, _dist_base + 0xf10); // sgi pending clear
  Io::write<Mword>(0xffffffff, _dist_base + 0xf14); // sgi pending clear
  Io::write<Mword>(0xffffffff, _dist_base + 0xf18); // sgi pending clear
  Io::write<Mword>(0xffffffff, _dist_base + 0xf1c); // sgi pending clear

  for (unsigned i = 0; i < 32; i += 4)
    Io::write<Mword>(0xa0a0a0a0, _dist_base + DIST_PRI + i);

  if (Config_tz_sec)
    Io::write<Mword>(0x40404040, _dist_base + DIST_PRI + 8);

  Io::write<Mword>(CPU_CTRL_ENABLE | (Config_tz_sec ? CPU_CTRL_USE_FIQ_FOR_SEC : 0), _cpu_base + CPU_CTRL);
  Io::write<Mword>(0xf0, _cpu_base + CPU_PRIMASK);
}

PUBLIC
void
Gic::init_ap()
{
  cpu_init();
}

PUBLIC
unsigned
Gic::init(bool primary_gic, int nr_irqs_override = -1)
{
  if (!primary_gic)
    {
      cpu_init();
      return 0;
    }

  Io::write<Mword>(0, _dist_base + DIST_CTRL);

  unsigned num = hw_nr_irqs();
  if (nr_irqs_override != -1)
    num = nr_irqs_override;

  if (!Config_mxc_tzic)
    {
      unsigned int intmask = 1U << cxx::int_value<Cpu_phys_id>(Proc::cpu_id());
      intmask |= intmask << 8;
      intmask |= intmask << 16;

      for (unsigned i = 32; i < num; i += 16)
        Io::write<Mword>(0, _dist_base + DIST_CONFIG + i * 4 / 16);
      for (unsigned i = 32; i < num; i += 4)
        Io::write<Mword>(intmask, _dist_base + DIST_TARGET + i);
    }

  for (unsigned i = 32; i < num; i += 4)
    Io::write<Mword>(0xa0a0a0a0, _dist_base + DIST_PRI + i);

  for (unsigned i = 32; i < num; i += 32)
    Io::write<Mword>(0xffffffff, _dist_base + DIST_ENABLE_CLEAR + i * 4 / 32);

  Mword v = 0;
  if (Config_tz_sec || Config_mxc_tzic)
    v = 0xffffffff;

  for (unsigned i = 32; i < num; i += 32)
    Io::write<Mword>(v, _dist_base + GICD_IGROUPR + i / 8);

  Mword dist_enable = DIST_CTRL_ENABLE;
  if (Config_mxc_tzic && !Config_tz_sec)
    dist_enable |= MXC_TZIC_CTRL_NSEN | MXC_TZIC_CTRL_NSENMASK;

  for (unsigned i = 0; i < num; ++i)
    set_cpu(i, Cpu_number(0));

  Io::write<Mword>(dist_enable, _dist_base + DIST_CTRL);

  if (Config_mxc_tzic)
    {
      Io::write<Mword>(0x0, _dist_base + MXC_TZIC_SYNCCTRL);
      Io::write<Mword>(0xf0, _dist_base + MXC_TZIC_PRIOMASK);
    }
  else
    cpu_init();

  return num;
}

PUBLIC
Gic::Gic(Address cpu_base, Address dist_base, int nr_irqs_override = -1)
  : _cpu_base(cpu_base), _dist_base(dist_base)
{
  unsigned num = init(true, nr_irqs_override);

  printf("Number of IRQs available at this GIC: %d\n", num);

  Irq_chip_gen::init(num);
}

/**
 * \brief Create a GIC device that is a physical alias for the
 *        master GIC.
 */
PUBLIC inline
Gic::Gic(Address cpu_base, Address dist_base, Gic *master_mapping)
  : _cpu_base(cpu_base), _dist_base(dist_base)
{
  Irq_chip_gen::init(master_mapping->nr_irqs());
}

PUBLIC inline NEEDS["io.h"]
void Gic::disable_locked( unsigned irq )
{ Io::write<Mword>(1 << (irq % 32), _dist_base + DIST_ENABLE_CLEAR + (irq / 32) * 4); }

PUBLIC inline NEEDS["io.h"]
void Gic::enable_locked(unsigned irq, unsigned /*prio*/)
{ Io::write<Mword>(1 << (irq % 32), _dist_base + DIST_ENABLE_SET + (irq / 32) * 4); }

PUBLIC inline
void Gic::acknowledge_locked(unsigned irq)
{
  if (!Config_mxc_tzic)
    Io::write<Mword>(irq, _cpu_base + CPU_EOI);
}

PUBLIC
void
Gic::mask(Mword pin)
{
  assert (cpu_lock.test());
  disable_locked(pin);
}

PUBLIC
void
Gic::mask_and_ack(Mword pin)
{
  assert (cpu_lock.test());
  disable_locked(pin);
  acknowledge_locked(pin);
}

PUBLIC
void
Gic::ack(Mword pin)
{
  acknowledge_locked(pin);
}


PUBLIC
void
Gic::unmask(Mword pin)
{
  assert (cpu_lock.test());
  enable_locked(pin, 0xa);
}

PUBLIC
int
Gic::set_mode(Mword, Mode)
{
  return 0;
}

PUBLIC
bool
Gic::is_edge_triggered(Mword) const
{ return false; }

PUBLIC inline
void
Gic::hit(Upstream_irq const *u)
{
  Unsigned32 num = pending();
  if (EXPECT_FALSE(num == 0x3ff))
    return;

  handle_irq<Gic>(num, u);
}

PUBLIC static
void
Gic::cascade_hit(Irq_base *_self, Upstream_irq const *u)
{
  // this function calls some virtual functions that might be
  // ironed out
  Cascade_irq *self = nonull_static_cast<Cascade_irq*>(_self);
  Gic *gic = nonull_static_cast<Gic*>(self->child());
  Upstream_irq ui(self, u);
  gic->hit(&ui);
}

//-------------------------------------------------------------------
IMPLEMENTATION [arm && arm_em_tz]:

PUBLIC
bool
Gic::alloc(Irq_base *irq, Mword pin)
{
  if (Irq_chip_gen::alloc(irq, pin))
    {
      printf("GIC: Switching IRQ %ld to secure\n", pin);

      unsigned shift = (pin & 3) * 8;

      if (pin < 32)
        {
          assert(((Io::read<Mword>(_dist_base + GICD_IGROUPR) >> pin) & 1) == 0);
          assert(((Io::read<Mword>(_dist_base + DIST_PRI + (pin & ~3)) >> shift) & 0xff) == 0x40);
        }
      else
        {
          Io::clear<Mword>(1UL << (pin & 0x1f),
                           _dist_base + GICD_IGROUPR + (pin & ~0x1f) / 8);

          Io::modify<Mword>(0x40 << shift, 0xff << shift,
                            _dist_base + DIST_PRI + (pin & ~3));
        }
      return true;
    }
  return false;
}

PUBLIC
void
Gic::set_pending_irq(unsigned idx, Unsigned32 val)
{
  if (idx < 32)
    {
      Address o = _dist_base + idx * 4;
      Io::write<Mword>(val & Io::read<Mword>(o + GICD_IGROUPR),
                       o + DIST_SET_PENDING);
    }
}

//-------------------------------------------------------------------
IMPLEMENTATION [arm && !mp && pic_gic]:

PUBLIC
void
Gic::set_cpu(Mword, Cpu_number)
{}

PUBLIC inline NEEDS["io.h"]
Unsigned32 Gic::pending()
{
  if (Config_mxc_tzic)
    {
      Address a = _dist_base + MXC_TZIC_PND;
      for (unsigned g = 0; g < 128; g += 32, a += 4)
        {
          Mword v = Io::read<Mword>(a);
          if (v)
            return g + 31 - __builtin_clz(v);
        }
      return 0;
    }

  return Io::read<Mword>(_cpu_base + CPU_INTACK) & 0x3ff;
}

//-------------------------------------------------------------------
IMPLEMENTATION [arm && mp && pic_gic]:

#include "cpu.h"

PUBLIC inline NEEDS["io.h"]
Unsigned32 Gic::pending()
{
  Unsigned32 ack = Io::read<Mword>(_cpu_base + CPU_INTACK);

  // IPIs/SGIs need to take the whole ack value
  if ((ack & 0x3ff) < 16)
    Io::write<Mword>(ack, _cpu_base + CPU_EOI);

  return ack & 0x3ff;
}

PUBLIC inline NEEDS["cpu.h"]
void
Gic::set_cpu(Mword pin, Cpu_number cpu)
{
  Mword reg = _dist_base + DIST_TARGET + (pin & ~3);
  Mword val = Io::read<Mword>(reg);

  int shift = (pin % 4) * 8;
  unsigned pcpu = cxx::int_value<Cpu_phys_id>(Cpu::cpus.cpu(cpu).phys_id());
  val = (val & ~(0xf << shift)) | (1 << (pcpu + shift));

  Io::write<Mword>(val, reg);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [debug]:

PUBLIC
char const *
Gic::chip_type() const
{ return "GIC"; }
