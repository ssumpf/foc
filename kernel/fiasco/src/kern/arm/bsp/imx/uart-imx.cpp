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

IMPLEMENTATION [imx51]:

#include "uart_imx.h"

IMPLEMENT int Uart::irq() const { return 31; }

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_imx51 uart;
  return &uart;
}

IMPLEMENTATION [imx21 || imx35 || imx51]:

#include "mem_layout.h"

IMPLEMENT Address Uart::base() const { return Mem_layout::Uart_base; }
