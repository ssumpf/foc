IMPLEMENTATION [imx21]:

#include "uart_imx.h"

IMPLEMENT int Uart::irq() const { return 20; }

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_imx21 uart;
  return &uart;
}

IMPLEMENTATION [imx35]:

#include "uart_imx.h"

// uart-1: 45
// uart-2: 32
// uart-3: 18
IMPLEMENT int Uart::irq() const { return 45; }

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_imx35 uart;
  return &uart;
}

IMPLEMENTATION [imx51 || imx53]:

#include "uart_imx.h"

IMPLEMENT int Uart::irq() const { return 31; }

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_imx51 uart;
  return &uart;
}

IMPLEMENTATION [imx6]:

#include "uart_imx.h"

IMPLEMENT int Uart::irq() const
{ return CONFIG_PF_IMX_UART_NR + 57; }

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_imx6 uart;
  return &uart;
}

IMPLEMENTATION:

#include "mem_layout.h"

IMPLEMENT Address Uart::base() const { return Mem_layout::Uart_phys_base; }
