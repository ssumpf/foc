IMPLEMENTATION[uart_of && libuart]:
#include "ppc32/uart_of.h"
#include "boot_info.h"

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_of uart((unsigned long)Boot_info::get_prom());
  return &uart;
}

