INTERFACE:

// On ARM the MMIO for the uart is accessible before the MMU is fully up
EXTENSION class Kernel_uart { enum { Bsp_init_mode = Init_before_mmu }; };

IMPLEMENTATION [arm && sa1100 && serial]:

#include "mem_layout.h"

IMPLEMENT
bool Kernel_uart::startup(unsigned port, int /*irq*/)
{
  return Uart::startup(Mem_layout::Uart_base, 17);
}
