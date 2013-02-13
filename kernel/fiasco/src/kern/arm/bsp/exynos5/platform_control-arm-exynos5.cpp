INTERFACE [arm && mp && exynos5]:
#include "types.h"

IMPLEMENTATION [arm && mp && exynos5]:

#include "io.h"
#include "kmem.h"
#include "stdio.h"


PUBLIC static
void
Platform_control::boot_ap_cpus(Address phys_tramp_mp_addr)
{
  // Write start address to iRam base (0x2020000). This is checked by the app
  // cpus wihtin an wfe (wait-for event) loop.
  printf("START CPUs\n");
  Io::write<Mword>(phys_tramp_mp_addr, Kmem::Devices5_map_base + 0x20000);
  // wake-up cpus
  asm volatile("dsb; sev" : : : "memory");
}

