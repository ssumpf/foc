IMPLEMENTATION [arm && exynos5]:

#include "io.h"
#include "kmem.h"

void __attribute__ ((noreturn))
platform_reset(void)
{
  enum { PRM_RSTCTRL = Kmem::Devices1_map_base + 0x40400 };

  Io::write<Mword>(1, PRM_RSTCTRL);

  for (;;)
   ;
}
