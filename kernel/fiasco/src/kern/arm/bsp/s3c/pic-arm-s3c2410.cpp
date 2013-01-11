INTERFACE [arm && s3c2410]:

#include "kmem.h"

EXTENSION class Pic
{
public:
  enum
  {
    SRCPND    = Kmem::Pic_map_base + 0x00,
    INTMODE   = Kmem::Pic_map_base + 0x04,
    INTMSK    = Kmem::Pic_map_base + 0x08,
    PRIORITY  = Kmem::Pic_map_base + 0x0c,
    INTPND    = Kmem::Pic_map_base + 0x10,
    INTOFFSET = Kmem::Pic_map_base + 0x14,
    SUBSRCPND = Kmem::Pic_map_base + 0x18,
    INTSUBMSK = Kmem::Pic_map_base + 0x1c,
  };

  enum
  {
    MAIN_0        = 0,
    MAIN_EINT4_7  = 4,
    MAIN_EINT8_23 = 5,
    MAIN_UART2    = 15,
    MAIN_LCD      = 16,
    MAIN_UART1    = 23,
    MAIN_UART0    = 28,
    MAIN_ADC      = 31,
    MAIN_31       = 31,

    SUB_RXD0 = 0,
    SUB_TXD0 = 1,
    SUB_ERR0 = 2,
    SUB_RXD1 = 3,
    SUB_TXD1 = 4,
    SUB_ERR1 = 5,
    SUB_RXD2 = 6,
    SUB_TXD2 = 7,
    SUB_ERR2 = 8,
    SUB_TC   = 9,
    SUB_ADC  = 10,
  };

  enum // Interrupts
  {
    // EINT4_7
    EINT4  = 32,
    EINT7  = 35,
    // EINT8_23
    EINT8  = 36,
    EINT23 = 51,
    // UART2
    INT_UART2_ERR = 52,
    INT_UART2_RXD = 53,
    INT_UART2_TXD = 54,
    // LCD
    INT_LCD_FRSYN = 55,
    INT_LCD_FICNT = 56,
    // UART1
    INT_UART1_ERR = 57,
    INT_UART1_RXD = 58,
    INT_UART1_TXD = 59,
    // UART0
    INT_UART0_ERR = 60,
    INT_UART0_RXD = 61,
    INT_UART0_TXD = 62,
    // ADC
    INT_ADC = 63,
    INT_TC  = 64,
  };
};

// ---------------------------------------------------------------------
IMPLEMENTATION [arm && s3c2410]:

#include "config.h"
#include "io.h"
#include "irq_chip_generic.h"
#include "irq_mgr.h"

#include <cstdio>

class S3c_chip : public Irq_chip_gen
{
public:
  S3c_chip() : Irq_chip_gen(32) {}
  unsigned set_mode(Mword, unsigned) { return Irq_base::Trigger_level; }
  void set_cpu(Mword, unsigned) {}
};


PUBLIC
void
S3c_chip::mask(Mword irq)
{
  Mword mainirq;

  switch (irq)
    {
      case Pic::INT_TC:        Io::set<Mword>(1 << Pic::SUB_TC,   Pic::INTSUBMSK); mainirq = Pic::MAIN_ADC;   break;
      case Pic::INT_ADC:       Io::set<Mword>(1 << Pic::SUB_ADC,  Pic::INTSUBMSK); mainirq = Pic::MAIN_ADC;   break;
      case Pic::INT_UART0_RXD: Io::set<Mword>(1 << Pic::SUB_RXD0, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART0; break;
      case Pic::INT_UART0_TXD: Io::set<Mword>(1 << Pic::SUB_TXD0, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART0; break;
      case Pic::INT_UART0_ERR: Io::set<Mword>(1 << Pic::SUB_ERR0, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART0; break;
      case Pic::INT_UART1_RXD: Io::set<Mword>(1 << Pic::SUB_RXD1, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART1; break;
      case Pic::INT_UART1_TXD: Io::set<Mword>(1 << Pic::SUB_TXD1, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART1; break;
      case Pic::INT_UART1_ERR: Io::set<Mword>(1 << Pic::SUB_ERR1, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART1; break;
      case Pic::INT_UART2_RXD: Io::set<Mword>(1 << Pic::SUB_RXD2, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART2; break;
      case Pic::INT_UART2_TXD: Io::set<Mword>(1 << Pic::SUB_TXD2, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART2; break;
      case Pic::INT_UART2_ERR: Io::set<Mword>(1 << Pic::SUB_ERR2, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART2; break;
      default:
         if (irq > 31)
           return; // XXX: need to add other cases
         mainirq = irq;
    };

  Io::set<Mword>(1 << mainirq, Pic::INTMSK);
}

PUBLIC
void
S3c_chip::unmask(Mword irq)
{
  int mainirq;

  switch (irq)
    {
      case Pic::INT_TC:        Io::clear<Mword>(1 << Pic::SUB_TC,   Pic::INTSUBMSK); mainirq = Pic::MAIN_ADC;   break;
      case Pic::INT_ADC:       Io::clear<Mword>(1 << Pic::SUB_ADC,  Pic::INTSUBMSK); mainirq = Pic::MAIN_ADC;   break;
      case Pic::INT_UART0_RXD: Io::clear<Mword>(1 << Pic::SUB_RXD0, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART0; break;
      case Pic::INT_UART0_TXD: Io::clear<Mword>(1 << Pic::SUB_TXD0, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART0; break;
      case Pic::INT_UART0_ERR: Io::clear<Mword>(1 << Pic::SUB_ERR0, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART0; break;
      case Pic::INT_UART1_RXD: Io::clear<Mword>(1 << Pic::SUB_RXD1, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART1; break;
      case Pic::INT_UART1_TXD: Io::clear<Mword>(1 << Pic::SUB_TXD1, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART1; break;
      case Pic::INT_UART1_ERR: Io::clear<Mword>(1 << Pic::SUB_ERR1, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART1; break;
      case Pic::INT_UART2_RXD: Io::clear<Mword>(1 << Pic::SUB_RXD2, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART2; break;
      case Pic::INT_UART2_TXD: Io::clear<Mword>(1 << Pic::SUB_TXD2, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART2; break;
      case Pic::INT_UART2_ERR: Io::clear<Mword>(1 << Pic::SUB_ERR2, Pic::INTSUBMSK); mainirq = Pic::MAIN_UART2; break;
      default:
         if (irq > 31)
           return; // XXX: need to add other cases
         mainirq = irq;
    };

  Io::clear<Mword>(1 << mainirq, Pic::INTMSK);
}

PUBLIC
void
S3c_chip::ack(Mword irq)
{
  int mainirq;

  switch (irq)
    {
      case Pic::INT_TC:        Io::write<Mword>(1 << Pic::SUB_TC,   Pic::SUBSRCPND); mainirq = Pic::MAIN_ADC;   break;
      case Pic::INT_ADC:       Io::write<Mword>(1 << Pic::SUB_ADC,  Pic::SUBSRCPND); mainirq = Pic::MAIN_ADC;   break;
      case Pic::INT_UART0_RXD: Io::write<Mword>(1 << Pic::SUB_RXD0, Pic::SUBSRCPND); mainirq = Pic::MAIN_UART0; break;
      case Pic::INT_UART0_TXD: Io::write<Mword>(1 << Pic::SUB_TXD0, Pic::SUBSRCPND); mainirq = Pic::MAIN_UART0; break;
      case Pic::INT_UART0_ERR: Io::write<Mword>(1 << Pic::SUB_ERR0, Pic::SUBSRCPND); mainirq = Pic::MAIN_UART0; break;
      case Pic::INT_UART1_RXD: Io::write<Mword>(1 << Pic::SUB_RXD1, Pic::SUBSRCPND); mainirq = Pic::MAIN_UART1; break;
      case Pic::INT_UART1_TXD: Io::write<Mword>(1 << Pic::SUB_TXD1, Pic::SUBSRCPND); mainirq = Pic::MAIN_UART1; break;
      case Pic::INT_UART1_ERR: Io::write<Mword>(1 << Pic::SUB_ERR1, Pic::SUBSRCPND); mainirq = Pic::MAIN_UART1; break;
      case Pic::INT_UART2_RXD: Io::write<Mword>(1 << Pic::SUB_RXD2, Pic::SUBSRCPND); mainirq = Pic::MAIN_UART2; break;
      case Pic::INT_UART2_TXD: Io::write<Mword>(1 << Pic::SUB_TXD2, Pic::SUBSRCPND); mainirq = Pic::MAIN_UART2; break;
      case Pic::INT_UART2_ERR: Io::write<Mword>(1 << Pic::SUB_ERR2, Pic::SUBSRCPND); mainirq = Pic::MAIN_UART2; break;
      default:
         if (irq > 31)
           return; // XXX: need to add other cases
        mainirq = irq;
    };

  Io::write<Mword>(1 << mainirq, Pic::SRCPND); // only 1s are set to 0
  Io::write<Mword>(1 << mainirq, Pic::INTPND); // clear pending interrupt
}

PUBLIC
void
S3c_chip::mask_and_ack(Mword irq)
{
  assert(cpu_lock.test());
  mask(irq);
  ack(irq);
}


static Static_object<Irq_mgr_single_chip<S3c_chip> > mgr;


IMPLEMENT FIASCO_INIT
void Pic::init()
{
  Irq_mgr::mgr = mgr.construct();

  Io::write<Mword>(0xffffffff, INTMSK); // all masked
  Io::write<Mword>(0x7fe, INTSUBMSK);   // all masked
  Io::write<Mword>(0, INTMODE);         // all IRQs, no FIQs
  Io::write<Mword>(Io::read<Mword>(SRCPND), SRCPND); // clear source pending
  Io::write<Mword>(Io::read<Mword>(SUBSRCPND), SUBSRCPND); // clear sub src pnd
  Io::write<Mword>(Io::read<Mword>(INTPND), INTPND); // clear pending interrupt
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
Unsigned32 Pic::pending()
{
  int mainirq = Io::read<Mword>(INTOFFSET);

  switch (mainirq)
    {
    case MAIN_ADC:
	{
	  int subirq = Io::read<Mword>(SUBSRCPND);
	  if ((1 << SUB_ADC) & subirq)
	    return INT_ADC;
	  else if ((1 << SUB_TC) & subirq)
	    return INT_TC;
	}
      break;
    // more: tbd
    default:
      return mainirq;
    }
  return 32;
}

extern "C"
void irq_handler()
{
  Unsigned32 i = Pic::pending();
  if (i != 32)
    mgr->c.handle_irq<S3c_chip>(i, 0);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [debug && s3c2410]:

PUBLIC
char const *
S3c_chip::chip_type() const
{ return "HW S3C2410 IRQ"; }

