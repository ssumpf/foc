IMPLEMENTATION:

#include "uart_s3c2410.h"
#include "kmem.h"
#include "mem_layout.h"
#include "platform.h"

IMPLEMENT_DEFAULT Address Uart::base() const
{ return Mem_layout::Uart_phys_base + Platform::uart_nr() * 0x10000; }

IMPLEMENT_DEFAULT int Uart::irq() const
{
  if (Platform::is_5250())
    return 51 + 32 + Platform::uart_nr();
  if (Platform::gic_ext())
    return 52 + 32 + Platform::uart_nr();
  return 96 + 8 * 26 + Platform::uart_nr();
}

IMPLEMENT_DEFAULT L4::Uart *Uart::uart()
{
  static L4::Uart_s5pv210 uart;
  return &uart;
}

