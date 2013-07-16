INTERFACE [exynos]:

#include "l4_types.h"

class Exynos_smc
{
public:
  static void cpuboot(Mword, unsigned);
  static void l2cache_setup(unsigned, unsigned, unsigned, Mword, Mword, Mword);
};

// ------------------------------------------------------------------------
INTERFACE [exynos && arm_em_ns && arm_smif_mc]:

#include "types.h"

EXTENSION class Exynos_smc
{
private:
  enum Command : Smword
  {
    Init       = -1,
    Info       = -2,
    Sleep      = -3,
    Cpu1boot   = -4,
    Cpu0aftr   = -5,
    C15resume  = -11,
    L2x0ctrl   = -21,
    L2x0setup1 = -22,
    L2x0setup2 = -23,
    L2x0invall = -24,
    L2x0debug  = -25,
    Reg        = -101,
  };

  static int call(Command cmd, Mword a1 = 0, Mword a2 = 0, Mword a3 = 0)
  {
    register Mword _cmd asm("r0") = cmd;
    register Mword _a1  asm("r1") = a1;
    register Mword _a2  asm("r2") = a2;
    register Mword _a3  asm("r3") = a3;

    asm volatile("dmb   \t\n" // Errata 766421
                 "smc 0 \t\n"
                 "dmb   \t\n" // Errata 766421
                 : "=r" (_cmd)
                 : "r" (_cmd), "r" (_a1), "r" (_a2), "r" (_a3)
                 : "memory", "cc");
    return _cmd;
  }
};

// ------------------------------------------------------------------------
IMPLEMENTATION [exynos && arm_em_ns && arm_smif_mc]:

#include "mem_space.h"
#include "mem_unit.h"
#include <cstdio>

IMPLEMENT
void
Exynos_smc::cpuboot(Mword, unsigned cpu)
{
  call(Cpu1boot, cpu);
}

IMPLEMENT
void
Exynos_smc::l2cache_setup(unsigned tag_lat, unsigned data_lat,
                          unsigned prefctrl, Mword setup2val,
                          Mword aux_val, Mword aux_mask)
{
  call(L2x0setup1, tag_lat, data_lat, prefctrl);
  call(L2x0setup2, setup2val, aux_val, aux_mask);
  call(L2x0invall);
  call(L2x0ctrl, 1);
}

// ------------------------------------------------------------------------
IMPLEMENTATION [exynos && !arm_em_ns]:

IMPLEMENT inline
void
Exynos_smc::cpuboot(Mword, unsigned)
{}

IMPLEMENT inline
void
Exynos_smc::l2cache_setup(unsigned, unsigned, unsigned, Mword, Mword, Mword)
{}
