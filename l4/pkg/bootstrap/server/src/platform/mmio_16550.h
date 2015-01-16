#include <startup.h>
#include "support.h"
#include <l4/drivers/uart_pxa.h>

static void setup_16550_mmio_uart()
{
  kuart.access_type = L4_kernel_options::Uart_type_mmio;
  static L4::Uart_16550 _uart(kuart.base_baud);
  static L4::Io_register_block_mmio r(kuart.base_address, kuart.reg_shift);
  _uart.startup(&r);
  _uart.change_mode(3, kuart.baud);
  set_stdio_uart(&_uart);

  kuart_flags |=   L4_kernel_options::F_uart_base
                 | L4_kernel_options::F_uart_baud;

  if (kuart.irqno != L4_kernel_options::Uart_irq_none)
    kuart_flags |= L4_kernel_options::F_uart_irq;
}
