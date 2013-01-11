INTERFACE [16550 && kirkwood]:

EXTENSION class Uart
{
public:
  enum {
    Base_rate     = 200000000 / 16,
    Base_ier_bits = 1 << 6,

    Access_shift  = 2,
  };
};

IMPLEMENTATION [16550 && kirkwood]:

IMPLEMENT inline NEEDS[Uart::mcr, Uart::ier]
void Uart::enable_rcv_irq()
{
  ier(ier() | 1);
}
