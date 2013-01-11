INTERFACE [sa1100]:

#include "types.h"

template< unsigned long Hw_regs_base >
class Sa1100_generic 
{
public:
  enum {
    RSRR = Hw_regs_base + 0x030000,

    /* interrupt controller */
    ICIP = Hw_regs_base + 0x050000,
    ICMR = Hw_regs_base + 0x050004,
    ICLR = Hw_regs_base + 0x050008,
    ICCR = Hw_regs_base + 0x05000c,
    ICFP = Hw_regs_base + 0x050010,
    ICPR = Hw_regs_base + 0x050020,

    /* OS Timer */
    OSMR0 = Hw_regs_base + 0x000000,
    OSMR1 = Hw_regs_base + 0x000004,
    OSMR2 = Hw_regs_base + 0x000008,
    OSMR3 = Hw_regs_base + 0x00000c,
    OSCR  = Hw_regs_base + 0x000010,
    OSSR  = Hw_regs_base + 0x000014,
    OWER  = Hw_regs_base + 0x000018,
    OIER  = Hw_regs_base + 0x00001c,

    RSRR_SWR = 1,
  };

  static inline void  hw_reg( Mword value, Mword reg );
  static inline Mword hw_reg( Mword reg );
};

//---------------------------------------------------------------------------
IMPLEMENTATION [sa1100]:

IMPLEMENT inline
template< unsigned long Hw_regs_base >
void Sa1100_generic<Hw_regs_base>::hw_reg( Mword value, Mword reg )
{
  *(Mword volatile*)reg = value;
}

IMPLEMENT inline
template< unsigned long Hw_regs_base >
Mword Sa1100_generic<Hw_regs_base>::hw_reg( Mword reg )
{
  return *(Mword volatile*)reg;
}

