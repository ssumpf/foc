INTERFACE [arm && kirkwood]:

#include "kmem.h"

class Irq_base;

EXTENSION class Pic
{
public:
  enum
  {
    Main_Irq_cause_low_reg     = Mem_layout::Pic_map_base + 0x20200,
    Main_Irq_mask_low_reg      = Mem_layout::Pic_map_base + 0x20204,
    Main_Fiq_mask_low_reg      = Mem_layout::Pic_map_base + 0x20208,
    Endpoint_irq_mask_low_reg  = Mem_layout::Pic_map_base + 0x2020c,
    Main_Irq_cause_high_reg    = Mem_layout::Pic_map_base + 0x20210,
    Main_Irq_mask_high_reg     = Mem_layout::Pic_map_base + 0x20214,
    Main_Fiq_mask_high_reg     = Mem_layout::Pic_map_base + 0x20218,
    Endpoint_irq_mask_high_reg = Mem_layout::Pic_map_base + 0x2021c,

    Bridge_int_num = 1,
  };
};

//-------------------------------------------------------------------
IMPLEMENTATION [arm && kirkwood]:

#include "config.h"
#include "initcalls.h"
#include "io.h"
#include "irq_chip_generic.h"
#include "irq_mgr.h"

class Irq_chip_kirkwood : public Irq_chip_gen
{
public:
  Irq_chip_kirkwood() : Irq_chip_gen(64) {}
  unsigned set_mode(Mword, unsigned) { return Irq_base::Trigger_level; }
  void set_cpu(Mword, unsigned) {}
  void ack(Mword) { /* ack is empty */ }
};

PUBLIC
void
Irq_chip_kirkwood::mask(Mword irq)
{
  assert (cpu_lock.test());
  Io::clear<Unsigned32>(1 << (irq & 0x1f),
			Pic::Main_Irq_mask_low_reg + ((irq & 0x20) >> 1));
}

PUBLIC
void
Irq_chip_kirkwood::mask_and_ack(Mword irq)
{
  assert(cpu_lock.test());
  mask(irq);
  // ack is empty
}

PUBLIC
void
Irq_chip_kirkwood::unmask(Mword irq)
{
  assert(cpu_lock.test());
  Io::set<Unsigned32>(1 << (irq & 0x1f),
                      Pic::Main_Irq_mask_low_reg + ((irq & 0x20) >> 1));
}

static Static_object<Irq_mgr_single_chip<Irq_chip_kirkwood> > mgr;

IMPLEMENT FIASCO_INIT
void Pic::init()
{
  Irq_mgr::mgr = mgr.construct();

  // Disable all interrupts
  Io::write<Unsigned32>(0U, Main_Irq_mask_low_reg);
  Io::write<Unsigned32>(0U, Main_Fiq_mask_low_reg);
  Io::write<Unsigned32>(0U, Main_Irq_mask_high_reg);
  Io::write<Unsigned32>(0U, Main_Fiq_mask_high_reg);

  // enable bridge (chain) IRQ
  Io::set<Unsigned32>(1 << Bridge_int_num, Main_Irq_mask_low_reg);
}

IMPLEMENT inline
Pic::Status Pic::disable_all_save()
{ return 0; }

IMPLEMENT inline
void Pic::restore_all(Status)
{}

PUBLIC static inline NEEDS["io.h"]
Unsigned32 Irq_chip_kirkwood::pending()
{
  Unsigned32 v;

  v = Io::read<Unsigned32>(Pic::Main_Irq_cause_low_reg);
  if (v & 1)
    {
      v = Io::read<Unsigned32>(Pic::Main_Irq_cause_high_reg);
      for (int i = 1; i < 32; ++i)
	if ((1 << i) & v)
	  return 32 + i;
    }
  for (int i = 1; i < 32; ++i)
    if ((1 << i) & v)
      return i;

  return 64;
}

extern "C"
void irq_handler()
{
  Unsigned32 i;
  while ((i = Irq_chip_kirkwood::pending()) < 64)
    mgr->c.handle_irq<Irq_chip_kirkwood>(i, 0);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [debug && kirkwood]:

PUBLIC
char const *
Irq_chip_kirkwood::chip_type() const
{ return "HW Kirkwood IRQ"; }
