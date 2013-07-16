INTERFACE [16550-{ia32,amd64}]:

EXTENSION class Uart
{
public:
  enum {
    Base_rate     = 115200,
    Base_ier_bits = 0,

    Access_shift  = 0,
  };
};


INTERFACE[16550]:

#include "types.h"

/**
 * 16550 implementation of the UART interface.
 */
EXTENSION class Uart
{
public:

  /**
   * Start this serial port for I/O.
   * @param port the I/O port base address.
   * @param irq the IRQ assigned to this port, -1 if none.
   */
  bool startup(Address port, int irq);

  enum {
    PAR_NONE = 0x00,
    PAR_EVEN = 0x18,
    PAR_ODD  = 0x08,
    DAT_5    = 0x00,
    DAT_6    = 0x01,
    DAT_7    = 0x02,
    DAT_8    = 0x03,
    STOP_1   = 0x00,
    STOP_2   = 0x04,

    MODE_8N1 = PAR_NONE | DAT_8 | STOP_1,
    MODE_7E1 = PAR_EVEN | DAT_7 | STOP_1,

  // these two values are to leave either mode
  // or baud rate unchanged on a call to change_mode
    MODE_NC  = 0x1000000,
    BAUD_NC  = 0x1000000,
  };

private:

  enum Registers {
    TRB      = 0, // Transmit/Receive Buffer  (read/write)
    BRD_LOW  = 0, // Baud Rate Divisor LSB if bit 7 of LCR is set  (read/write)
    IER      = 1, // Interrupt Enable Register  (read/write)
    BRD_HIGH = 1, // Baud Rate Divisor MSB if bit 7 of LCR is set  (read/write)
    IIR      = 2, // Interrupt Identification Register  (read only)
    FCR      = 2, // 16550 FIFO Control Register  (write only)
    LCR      = 3, // Line Control Register  (read/write)
    MCR      = 4, // Modem Control Register  (read/write)
    LSR      = 5, // Line Status Register  (read only)
    MSR      = 6, // Modem Status Register  (read only)
    SPR      = 7, // Scratch Pad Register  (read/write)
  };

  Address port;
  int _irq;
};


IMPLEMENTATION[16550]:

#include "io.h"
#include "processor.h"


IMPLEMENT
Uart::Uart() : port(~0U), _irq(-1)
{}

IMPLEMENT
Uart::~Uart()
{}


PRIVATE inline NEEDS["io.h"]
void Uart::outb( Unsigned8 b, Registers reg )
{
  Io::out8(b, port + (reg << Access_shift));
}

PRIVATE inline NEEDS["io.h"]
Unsigned8 Uart::inb( Registers reg ) const
{
  return Io::in8(port + (reg << Access_shift));
}


PRIVATE inline NEEDS[Uart::outb]
void Uart::mcr(Unsigned8 b)
{
  outb(b, MCR);
}

PRIVATE inline NEEDS[Uart::inb]
Unsigned8 Uart::mcr() const
{
  return inb(MCR);
}

PRIVATE inline NEEDS[Uart::outb]
void Uart::fcr(Unsigned8 b)
{
  outb(b, FCR);
}

PRIVATE inline NEEDS[Uart::outb]
void Uart::lcr(Unsigned8 b)
{
  outb(b, LCR);
}

PRIVATE inline NEEDS[Uart::inb]
Unsigned8 Uart::lcr() const
{
  return inb(LCR);
}

PRIVATE inline NEEDS[Uart::outb]
void Uart::ier(Unsigned8 b)
{
  outb(b, IER);
}

PRIVATE inline NEEDS[Uart::inb]
Unsigned8 Uart::ier() const
{
  return inb(IER);
}

PRIVATE inline NEEDS[Uart::inb]
Unsigned8 Uart::iir() const
{
  return inb(IIR);
}

PRIVATE inline NEEDS[Uart::inb]
Unsigned8 Uart::msr() const
{
  return inb(MSR);
}

PRIVATE inline NEEDS[Uart::inb]
Unsigned8 Uart::lsr() const
{
  return inb(LSR);
}

PRIVATE inline NEEDS[Uart::outb]
void Uart::trb(Unsigned8 b)
{
  outb(b, TRB);
}

PRIVATE inline NEEDS[Uart::inb]
Unsigned8 Uart::trb() const
{
  return inb(TRB);
}

PRIVATE
bool Uart::valid()
{
  Unsigned8 scratch, scratch2, scratch3;

  scratch = ier();
  ier(0x00);

  Io::iodelay();

  scratch2 = ier();
  ier(0x0f);
  Io::iodelay();

  scratch3 = ier();
  ier(scratch);

  return scratch2 == 0x00 && scratch3 == 0x0f;
}

IMPLEMENT
bool Uart::startup(Address _port, int __irq)
{
  port = _port;
  _irq  = __irq;

  Proc::Status o = Proc::cli_save();

  if (!valid())
    {
      Proc::sti_restore(o);
      fail();
      return false;
    }

  ier(Base_ier_bits);/* disable all rs-232 interrupts */
  mcr(0x0b);         /* out2, rts, and dtr enabled */
  fcr(7);            /* enable and clear rcv+xmit fifo */
  lcr(0);            /* clear line control register */

  /* clearall interrupts */
  /*read*/ msr(); /* IRQID 0*/
  /*read*/ iir(); /* IRQID 1*/
  /*read*/ trb(); /* IRQID 2*/
  /*read*/ lsr(); /* IRQID 3*/

  while(lsr() & 1/*DATA READY*/) /*read*/ trb();
  Proc::sti_restore(o);
  return true;
}


IMPLEMENT
void Uart::shutdown()
{
  Proc::Status o = Proc::cli_save();
  mcr(0x06);
  fcr(0);
  lcr(0);
  ier(0);
  Proc::sti_restore(o);
}

IMPLEMENT
bool Uart::change_mode(TransferMode m, BaudRate r)
{
  Proc::Status o = Proc::cli_save();
  Unsigned8 old_lcr = lcr();
  if(r != BAUD_NC) {
    lcr(old_lcr | 0x80/*DLAB*/);
    Unsigned16 divisor = Base_rate/r;
    trb( divisor & 0x0ff );        /* BRD_LOW  */
    ier( (divisor >> 8) & 0x0ff ); /* BRD_HIGH */
    lcr(old_lcr);
  }
  if( m != MODE_NC ) {
    lcr( m & 0x07f );
  }

  Proc::sti_restore(o);
  return true;
}

IMPLEMENT
Uart::TransferMode Uart::get_mode()
{
  return lcr() & 0x7f;
}

IMPLEMENT
int Uart::write(char const *s, size_t count)
{
  /* disable uart irqs */
  Unsigned8 old_ier;
  old_ier = ier();
  ier(old_ier & ~0x0f);

  /* transmission */
  for (unsigned i = 0; i < count; i++)
    {
      while (!(lsr() & 0x20 /* THRE */))
	;
      trb(s[i]);
    }

  /* wait till everything is transmitted */
  while (!(lsr() & 0x40 /* TSRE */))
    ;

  ier(old_ier);
  return count;
}

IMPLEMENT
int Uart::getchar(bool blocking)
{
  if (!blocking && !(lsr() & 1 /* DATA READY */))
    return -1;

  Unsigned8 old_ier, ch;
  old_ier = ier();
  ier(old_ier & ~0x0f);
  while (!(lsr() & 1 /* DATA READY */))
    ;
  ch = trb();
  ier(old_ier);
  return ch;
}

IMPLEMENT
int Uart::char_avail() const
{
  if (lsr() & 1 /* DATA READY */)
    return 1;

  return 0;
}


IMPLEMENT inline
int Uart::irq() const
{
  return _irq;
}

IMPLEMENT inline NEEDS[Uart::ier]
void Uart::disable_rcv_irq()
{
  ier(ier() & ~1);
}


// ------------------------------------------------------------------------
IMPLEMENTATION [16550-{ia32,amd64}]:

IMPLEMENT inline NEEDS[Uart::ier]
void Uart::enable_rcv_irq()
{
  ier(ier() | 1);
}
