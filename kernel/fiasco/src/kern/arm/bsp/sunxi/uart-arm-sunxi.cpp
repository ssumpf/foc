IMPLEMENTATION [16550 && sunxi]:

struct Sunxi_uart_16550 : L4::Uart_16550
{
  L4::Io_register_block const *regs() { return _regs; }
};

PRIVATE inline NOEXPORT
Unsigned8 Uart::usr() const
{
  return static_cast<Sunxi_uart_16550*>(uart())->regs()->read<Unsigned8>(0x1f);
}

PRIVATE inline NOEXPORT
Unsigned8 Uart::iir() const
{
  return static_cast<Sunxi_uart_16550*>(uart())->regs()->read<Unsigned8>(0x02);
}

PRIVATE inline NOEXPORT
void Uart::lcr(Unsigned8 val) const
{
  static_cast<Sunxi_uart_16550*>(uart())->regs()->write<Unsigned8>(val, 0x03);
}

PRIVATE inline NOEXPORT
Unsigned8 Uart::lcr() const
{
  return static_cast<Sunxi_uart_16550*>(uart())->regs()->read<Unsigned8>(0x03);
}

IMPLEMENT
void
Uart::irq_ack()
{
  if ((iir() & 7) == 7)
    {
      Unsigned8 l = lcr();
      Unsigned8 v = usr();
      asm volatile("" : : "r" (v) : "memory");
      lcr(l);
    }
}
