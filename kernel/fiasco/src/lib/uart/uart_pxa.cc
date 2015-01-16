/*!
 * \file   uart_pxa.cc
 * \brief  PXA UART implementation
 *
 * \date   2008-01-04
 * \author Adam Lackorznynski <adam@os.inf.tu-dresden.de>
 *         Alexander Warg <alexander.warg@os.inf.tu-dresden.de>
 *
 */
/*
 * (c) 2008-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#include "uart_pxa.h"
#include "poll_timeout_counter.h"

namespace L4
{
  enum Registers
  {
    TRB      = 0x00, // Transmit/Receive Buffer  (read/write)
    BRD_LOW  = 0x00, // Baud Rate Divisor LSB if bit 7 of LCR is set  (read/write)
    IER      = 0x01, // Interrupt Enable Register  (read/write)
    BRD_HIGH = 0x01, // Baud Rate Divisor MSB if bit 7 of LCR is set  (read/write)
    IIR      = 0x02, // Interrupt Identification Register  (read only)
    FCR      = 0x02, // 16550 FIFO Control Register  (write only)
    LCR      = 0x03, // Line Control Register  (read/write)
    MCR      = 0x04, // Modem Control Register  (read/write)
    LSR      = 0x05, // Line Status Register  (read only)
    MSR      = 0x06, // Modem Status Register  (read only)
    SPR      = 0x07, // Scratch Pad Register  (read/write)
  };

  enum
  {
    Base_ier_bits = 1 << 6, // pxa only?
  };

  bool Uart_16550::startup(Io_register_block const *regs)
  {
    _regs = regs;

    char scratch, scratch2, scratch3;

    scratch = _regs->read<unsigned char>(IER);
    _regs->write<unsigned char>(IER, 0);

    _regs->delay();

    scratch2 = _regs->read<unsigned char>(IER);
    _regs->write<unsigned char>(IER, 0xf);

    _regs->delay();

    scratch3 = _regs->read<unsigned char>(IER);
    _regs->write<unsigned char>(IER, scratch);

    if (!(scratch2 == 0x00 && scratch3 == 0x0f))
      return false;  // this is not the uart

    _regs->write<unsigned char>(IER, Base_ier_bits);/* disable all rs-232 interrupts */
    _regs->write<unsigned char>(MCR, 0xb);          /* out2, rts, and dtr enabled */
    _regs->write<unsigned char>(FCR, 7);            /* enable fifo + clear rcv+xmit fifo */
    _regs->write<unsigned char>(LCR, 0);            /* clear line control register */

    /* clearall interrupts */
    _regs->read<unsigned char>(MSR); /* IRQID 0*/
    _regs->read<unsigned char>(IIR); /* IRQID 1*/
    _regs->read<unsigned char>(TRB); /* IRQID 2*/
    _regs->read<unsigned char>(LSR); /* IRQID 3*/

    Poll_timeout_counter i(5000000);
    while (i.test(_regs->read<unsigned char>(LSR) & 1/*DATA READY*/))
      _regs->read<unsigned char>(TRB);

    return true;
  }

  void Uart_16550::shutdown()
  {
    _regs->write<unsigned char>(MCR, 6);
    _regs->write<unsigned char>(FCR, 0);
    _regs->write<unsigned char>(LCR, 0);
    _regs->write<unsigned char>(IER, 0);
  }

  bool Uart_16550::change_mode(Transfer_mode m, Baud_rate r)
  {
    unsigned long old_lcr = _regs->read<unsigned char>(LCR);
    if(r != BAUD_NC) {
      unsigned short divisor = _base_rate / r;
      _regs->write<unsigned char>(LCR, old_lcr | 0x80/*DLAB*/);
      _regs->write<unsigned char>(TRB, divisor & 0x0ff);        /* BRD_LOW  */
      _regs->write<unsigned char>(IER, (divisor >> 8) & 0x0ff); /* BRD_HIGH */
      _regs->write<unsigned char>(LCR, old_lcr);
    }
    if (m != MODE_NC)
      _regs->write<unsigned char>(LCR, m & 0x07f);

    return true;
  }

  int Uart_16550::get_char(bool blocking) const
  {
    char old_ier, ch;

    if (!blocking && !char_avail())
      return -1;

    old_ier = _regs->read<unsigned char>(IER);
    _regs->write<unsigned char>(IER, old_ier & ~0xf);
    while (!char_avail())
      ;
    ch = _regs->read<unsigned char>(TRB);
    _regs->write<unsigned char>(IER, old_ier);
    return ch;
  }

  int Uart_16550::char_avail() const
  {
    return _regs->read<unsigned char>(LSR) & 1; // DATA READY
  }

  void Uart_16550::out_char(char c) const
  {
    write(&c, 1);
  }

  int Uart_16550::write(char const *s, unsigned long count) const
  {
    /* disable uart irqs */
    char old_ier;
    unsigned i;
    old_ier = _regs->read<unsigned char>(IER);
    _regs->write<unsigned char>(IER, old_ier & ~0x0f);

    /* transmission */
    Poll_timeout_counter cnt(5000000);
    for (i = 0; i < count; i++) {
      cnt.set(5000000);
      while (cnt.test(!(_regs->read<unsigned char>(LSR) & 0x20 /* THRE */)))
        ;
      _regs->write<unsigned char>(TRB, s[i]);
    }

    /* wait till everything is transmitted */
    cnt.set(5000000);
    while (cnt.test(!(_regs->read<unsigned char>(LSR) & 0x40 /* TSRE */)))
      ;

    _regs->write<unsigned char>(IER, old_ier);
    return count;
  }

  bool Uart_16550::enable_rx_irq(bool enable)
  {
    unsigned char ier = _regs->read<unsigned char>(IER);
    bool res = ier & 1;
    if (enable)
      ier |= 1;
    else
      ier &= ~1;

    _regs->write<unsigned char>(IER, ier);
    return res;
  }
};
