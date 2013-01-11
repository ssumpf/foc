INTERFACE [16550 && tegra2]:

EXTENSION class Uart
{
public:
  enum {
    Base_rate     = 13478400,
    Base_ier_bits = 1 << 6,

    Access_shift  = 2,
  };
};

IMPLEMENTATION [16550 && tegra2]:

IMPLEMENT inline NEEDS[Uart::mcr, Uart::ier]
void Uart::enable_rcv_irq()
{
  ier(ier() | 1);
}
