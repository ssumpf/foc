IMPLEMENTATION [arm && exynos5]: // ------------------------------

IMPLEMENT int Uart::irq() const { return 32 + 53; }

IMPLEMENTATION: // --------------------------------------------------------

#include "mem_layout.h"
#include "uart_s3c2410.h"

IMPLEMENT Address Uart::base() const { return Mem_layout::Uart_base; }

IMPLEMENT L4::Uart *Uart::uart()
{
  static L4::Uart_s5pv210 uart;
  return &uart;
}
