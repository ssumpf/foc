INTERFACE [sparc && leon3]:

#include "types.h"

class Irq_chip_icu;

EXTENSION class Pic
{
private:
  /** Pic interrupt control registers (incomplete) */
  ///+0x00: Peripheral Interrupt Mask Register
  static Address per_mask() { return _pic_base; }
  ///+0x04: Peripheral Priority & HI/LO Select Register1
  static Address per_prio1() { return _pic_base + 0x04; }
  ///+0x08: Peripheral Priority & HI/LO Select Register2
  static Address per_prio2() { return _pic_base + 0x08; }
  ///+0x0c: Peripheral Priority & HI/LO Select Register3
  static Address per_prio3() { return _pic_base + 0x0c; }
  ///+0x10: External Enable & External Types Register
  static Address ext() { return _pic_base + 0x10; }
  ///+0x14: Critical Priority & Main Inter. Mask Register
  static Address main_mask() { return _pic_base + 0x14; }
  ///+0x18: Main Inter. Priority1
  static Address main_prio1() { return _pic_base + 0x18; }
  ///+0x1c: Main Inter. Priority2
  static Address main_prio2() { return _pic_base + 0x1c; }
  ///+0x24: PerStat, MainStat, CritStat Encoded Registers
  static Address stat() { return _pic_base + 0x24; }


  /** Interrupt lines (sdma is missing) */
  enum Pic_lines
  {
    IRQ_CRIT = 0x0, ///Critical line
    IRQ_MAIN = 0x1, ///Main line
    IRQ_PER  = 0x2, ///Periphal line
    IRQ_SHIFT = 0x4
  };

  enum Pic_num_per_line
  {
    NUM_CRIT = 4,
    NUM_MAIN = 17,
    NUM_PER  = 24,
  };

  /** Interrupt senses */
  enum Pic_sense
  {
    SENSE_LEVEL_HIGH   = 0,
    SENSE_EDGE_RISING  = 1,
    SENSE_EDGE_FALLING = 2,
    SENSE_LEVEL_LOW    = 3
  };

  static Address _pic_base;

public:
  enum { IRQ_MAX  = (IRQ_PER << IRQ_SHIFT) + NUM_PER};
  enum { No_irq_pending = ~0U };

  static Irq_chip_icu *main;
};

//------------------------------------------------------------------------------
IMPLEMENTATION [sparc && leon3]:

#include "boot_info.h"
#include "io.h"
#include "irq.h"
#include "irq_chip_generic.h"
#include "irq_mgr.h"
#include "panic.h"
#include "sparc_types.h"

#include <cassert>
#include <cstdio>

Irq_chip_icu *Pic::main;

class Chip : public Irq_chip_gen
{
public:
  Chip() : Irq_chip_gen(Pic::IRQ_MAX) {}
  int set_mode(Mword, Mode) { return 0; }
  bool is_edge_triggered(Mword) const { return false; }
  void set_cpu(Mword, Cpu_number) {}
};

PUBLIC
void
Chip::mask(Mword)
{
  assert(cpu_lock.test());
  //Pic::disable_locked(irq);
}

PUBLIC
void
Chip::ack(Mword)
{
  assert(cpu_lock.test());
  //Pic::acknowledge_locked(irq);
}

PUBLIC
void
Chip::mask_and_ack(Mword)
{
  assert(cpu_lock.test());
  //Pic::disable_locked(irq());
  //Pic::acknowledge_locked(irq());
}

PUBLIC
void
Chip::unmask(Mword)
{
  assert(cpu_lock.test());
  //Pic::enable_locked(irq());
}

IMPLEMENT FIASCO_INIT
void
Pic::init()
{
}

//------------------------------------------------------------------------------
/**
 * Irq number translations
 */
PUBLIC static inline
unsigned
Pic::irq_num(unsigned line, unsigned pic_num)
{ return (line << IRQ_SHIFT) | pic_num; }


PUBLIC static
int
Pic::get_irq_num(const char *name, const char *type)
{
  (void)name; (void)type;
  return -1;
}

PRIVATE static inline NEEDS[<cassert>]
unsigned
Pic::pic_line(unsigned irq_num)
{
  unsigned line = irq_num >> IRQ_SHIFT;
  assert(line < 3);
  return line;
}

PRIVATE static inline NEEDS[Pic::pic_line]
unsigned
Pic::pic_num(unsigned irq_num)
{
  unsigned line = pic_line(irq_num);
  unsigned num = irq_num & ~(~0U << IRQ_SHIFT);

  switch(line)
    {
    case IRQ_CRIT:
      assert(num < NUM_CRIT);
      break;
    case IRQ_MAIN:
      assert(num < NUM_MAIN);
      break;
    default:
      assert(num < NUM_PER);
    }

  return num;
}


//-------------------------------------------------------------------------------
/**
 * Interface implementation
 */
IMPLEMENT inline
void
Pic::block_locked (unsigned irq_num)
{
  disable_locked(irq_num);
}

IMPLEMENT inline NEEDS["io.h", Pic::pic_line, Pic::pic_num, Pic::set_stat_msb]
void
Pic::acknowledge_locked(unsigned irq_num)
{
  unsigned line = pic_line(irq_num);
  unsigned num  = pic_num(irq_num);

  if((line == IRQ_MAIN && (num >= 1 || num <= 3)) ||
     (line == IRQ_CRIT && num == 0))
    Io::set<Unsigned32>(1U << (27 - num), ext());

  set_stat_msb(irq_num);
}

PRIVATE static inline
void
Pic::dispatch_mask(unsigned irq_num, Address *addr, unsigned *bit_offs)
{
  switch(pic_line(irq_num))
    {
    case IRQ_MAIN:
      *addr = main_mask();
      *bit_offs = 16;
      break;
    case IRQ_PER:
      *addr = per_mask();
      *bit_offs = 31;
      break;
    default:
      panic("%s: Cannot handle IRQ %u", __PRETTY_FUNCTION__, irq_num);
    }
}

PRIVATE static inline
void
Pic::set_stat_msb(unsigned irq_num)
{
  switch(pic_line(irq_num))
    {
    case IRQ_MAIN:
      Io::set<Unsigned32>(0x00200000, stat());
      break;
    case IRQ_PER:
      Io::set<Unsigned32>(0x20000000, stat());
      break;
    default:
      panic("CRIT not implemented");
    }
}

IMPLEMENT inline NEEDS[Pic::pic_num, Pic::dispatch_mask]
void
Pic::disable_locked (unsigned irq_num)
{
  Address addr;
  unsigned bit_offs;
  dispatch_mask(irq_num, &addr, &bit_offs);
  Io::set<Unsigned32>(1U << (bit_offs - pic_num(irq_num)), addr);
}

IMPLEMENT inline NEEDS[Pic::dispatch_mask]
void
Pic::enable_locked (unsigned irq_num, unsigned /*prio*/)
{
  Address addr;
  unsigned bit_offs;
  dispatch_mask(irq_num, &addr, &bit_offs);

  Io::clear<Unsigned32>(1U << (bit_offs - pic_num(irq_num)), addr);
}

PUBLIC static inline NEEDS["panic.h"]
unsigned
Pic::nr_irqs()
{ return IRQ_MAX; }

PRIVATE static inline
Unsigned32
Pic::pending_per(Unsigned32 state)
{
  Unsigned32 irq = state >> 24 & 0x1f; //5 bit

  if(irq  == 0)
    panic("No support for bestcomm interrupt, yet\n");

  return irq_num(IRQ_PER, irq);
}

PRIVATE static inline
Unsigned32
Pic::pending_main(Unsigned32 state)
{
  Unsigned32 irq = state >> 16 & 0x1f;

  //low periphal
  if(irq == 4)
    return pending_per(state);

  return irq_num(IRQ_MAIN, irq);
}

PUBLIC static inline NEEDS[Pic::pending_main, Pic::pending_per]
Unsigned32
Pic::pending()
{
  Unsigned32 irq = No_irq_pending;
  return irq;
}

/**
 * disable interrupt lines [0-3]
 */
IMPLEMENT inline
Pic::Status
Pic::disable_all_save()
{
  Status s;
  return s;
}

IMPLEMENT inline
void
Pic::restore_all(Status s)
{
  Io::write<Unsigned32>(s, ext());
}

// ------------------------------------------------------------------------
IMPLEMENTATION [debug && sparc && leon3]:

PUBLIC
char const *
Chip::chip_type() const
{ return "HW Mpc52xx IRQ"; }
