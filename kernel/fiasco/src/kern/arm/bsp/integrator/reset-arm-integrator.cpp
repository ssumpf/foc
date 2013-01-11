IMPLEMENTATION [arm && integrator]:

#include "io.h"
#include "kmem.h"

void __attribute__ ((noreturn))
platform_reset(void)
{
  enum {
    HDR_CTRL_OFFSET = Kmem::Integrator_map_base + 0xc,
  };

  Io::write(1 << 3, HDR_CTRL_OFFSET);

  for (;;)
    ;
}
