INTERFACE:

class Kernel_uart
{
public:
  enum Init_mode
  {
    Init_before_mmu,
    Init_after_mmu
  };
  Kernel_uart();
  static void enable_rcv_irq();
};

INTERFACE [serial]:

#include "uart.h"
#include "std_macros.h"

/**
 * Glue between kernel and UART driver.
 */
EXTENSION class Kernel_uart : public Uart
{
private:
  /**
   * Prototype for the UART specific startup implementation.
   * @param uart, the instantiation to start.
   * @param port, the com port number.
   */
  bool startup(unsigned port, int irq=-1);
};

//---------------------------------------------------------------------------
IMPLEMENTATION [serial]:

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "filter_console.h"
#include "irq_chip.h"
#include "irq_mgr.h"
#include "kdb_ke.h"
#include "kernel_console.h"
#include "uart.h"
#include "config.h"
#include "kip.h"
#include "koptions.h"
#include "panic.h"
#include "vkey.h"

static Static_object<Filter_console> _fcon;
static Static_object<Kernel_uart> _kernel_uart;

PUBLIC static FIASCO_CONST
Uart *
Kernel_uart::uart()
{ return _kernel_uart; }

PUBLIC static
bool
Kernel_uart::init(Init_mode init_mode = Init_before_mmu)
{
  if ((int)init_mode != Bsp_init_mode)
    return false;

  if (Koptions::o()->opt(Koptions::F_noserial)) // do not use serial uart
    return true;

  _kernel_uart.construct();
  _fcon.construct(_kernel_uart);

  Kconsole::console()->register_console(_fcon, 0);
  return true;
}

IMPLEMENT
Kernel_uart::Kernel_uart()
{
  unsigned           n = Config::default_console_uart_baudrate;
  Uart::TransferMode m = Uart::MODE_8N1;
  unsigned long long p = Config::default_console_uart;
  int                i = -1;

  if (Koptions::o()->opt(Koptions::F_uart_baud))
    n = Koptions::o()->uart.baud;

  if (Koptions::o()->opt(Koptions::F_uart_base))
    p = Koptions::o()->uart.base_address;

  if (Koptions::o()->opt(Koptions::F_uart_irq))
    i = Koptions::o()->uart.irqno;

  if (!startup(p, i))
    printf("Comport/base 0x%04llx is not accepted by the uart driver!\n", p);
  else if (!change_mode(m, n))
    panic("Somthing is wrong with the baud rate (%d)!\n", n);
}


class Kuart_irq : public Irq_base
{
public:
  Kuart_irq() { hit_func = &handler_wrapper<Kuart_irq>; }
  void switch_mode(unsigned) {}
  void handle(Upstream_irq const *ui)
  {
    mask_and_ack();
    ui->ack();
    if (!Vkey::check_())
      kdb_ke("IRQ ENTRY");
    unmask();
  }
};


IMPLEMENT
void
Kernel_uart::enable_rcv_irq()
{
  static Kuart_irq uart_irq;
  if (Irq_mgr::mgr->alloc(&uart_irq, uart()->irq()))
    {
      uart_irq.unmask();
      uart()->enable_rcv_irq();
    }
}

//---------------------------------------------------------------------------
IMPLEMENTATION [!serial]:

PUBLIC static
bool
Kernel_uart::init(Init_mode = Init_before_mmu)
{ return false; }

IMPLEMENT inline
Kernel_uart::Kernel_uart()
{}

IMPLEMENT inline
void
Kernel_uart::enable_rcv_irq()
{}
