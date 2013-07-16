INTERFACE:

// On ARM the MMIO for the uart is accessible before the MMU is fully up
EXTENSION class Kernel_uart { enum { Bsp_init_mode = Init_before_mmu }; };

IMPLEMENTATION [arm && tegra2 && serial]:

#include "kmem.h"

IMPLEMENT
bool Kernel_uart::startup(unsigned, int)
{
  return Uart::startup(Kmem::mmio_remap(Mem_layout::Uart_phys_base) + 0x300 /* UARTD */, 122);
}

IMPLEMENTATION [arm && tegra3 && serial]:

#include "kmem.h"

IMPLEMENT
bool Kernel_uart::startup(unsigned, int)
{
  if (0)
    return Uart::startup(Kmem::mmio_remap(Mem_layout::Uart_phys_base), 68); // uarta
  return Uart::startup(Kmem::mmio_remap(Mem_layout::Uart_phys_base) + 0x200, 78); // uartc
}
