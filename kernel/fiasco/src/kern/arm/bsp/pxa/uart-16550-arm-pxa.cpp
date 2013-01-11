INTERFACE [16550 && pxa]:

EXTENSION class Uart
{
public:
  enum {
    Base_rate     = 921600,
    Base_ier_bits = 1 << 6,

    Access_shift  = 2,
  };
};

IMPLEMENTATION [16550 && pxa]:

IMPLEMENT inline NEEDS[Uart::mcr, Uart::ier]
void Uart::enable_rcv_irq()
{
  //mcr(mcr() & ~0x08); //XScale DOC is WRONG
  mcr(mcr() | 0x08);
  ier(ier() | 1);
}
