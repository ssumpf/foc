INTERFACE [arm && mp && omap4]:

#include "types.h"

class Boot_mp
{
};

IMPLEMENTATION [arm && mp && omap4]:

#include "io.h"
#include "kmem.h"

PRIVATE
void
Boot_mp::aux(unsigned cmd, Mword arg0, Mword arg1)
{
  register unsigned long r0  asm("r0")  = arg0;
  register unsigned long r1  asm("r1")  = arg1;
  register unsigned long r12 asm("r12") = cmd;

  asm volatile("dsb; smc #0"
               : : "r" (r0), "r" (r1), "r" (r12)
               : "r2", "r3", "r4", "r5", "r6",
                 "r7", "r8", "r9", "r10", "r11", "lr", "memory");
}

PUBLIC
void
Boot_mp::start_ap_cpus(Address phys_tramp_mp_addr)
{
  // two possibilities available, the memory mapped only in later board
  // revisions
  if (1)
    {
      enum {
        AUX_CORE_BOOT_0 = 0x104,
        AUX_CORE_BOOT_1 = 0x105,
      };
      aux(AUX_CORE_BOOT_1, phys_tramp_mp_addr, 0);
      asm volatile("dsb; sev" : : : "memory");
      aux(AUX_CORE_BOOT_0, 0x200, 0xfffffdff);
    }
  else
    {
      enum {
        AUX_CORE_BOOT_0 = Kmem::Devices2_map_base + 0x81800,
        AUX_CORE_BOOT_1 = Kmem::Devices2_map_base + 0x81804,
      };
      Io::write<Mword>(phys_tramp_mp_addr, AUX_CORE_BOOT_1);
      asm volatile("dsb; sev" : : : "memory");
      Io::write<Mword>((Io::read<Mword>(AUX_CORE_BOOT_0) & ~0xfffffdff) | 0x200,
                       AUX_CORE_BOOT_0);
    }
}

PUBLIC
void
Boot_mp::cleanup()
{}
