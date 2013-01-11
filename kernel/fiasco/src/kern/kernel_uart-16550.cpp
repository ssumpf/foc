INTERFACE[16550]:

// The port-based 16550 UART can be used before the MMU is initialized
EXTENSION class Kernel_uart { enum { Bsp_init_mode = Init_before_mmu }; };

IMPLEMENTATION[16550]:

IMPLEMENT
bool Kernel_uart::startup(unsigned port, int irq)
{
  return Uart::startup(port, irq);
}
