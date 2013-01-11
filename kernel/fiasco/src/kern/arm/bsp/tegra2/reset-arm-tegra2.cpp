IMPLEMENTATION [arm && tegra2]:

#include "io.h"
#include "kmem.h"

class Tegra2_reset
{
public:
  enum
  {
    RESET = Kmem::Clock_reset_map_base + 0x4,
  };
};

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && tegra2]:

void __attribute__ ((noreturn))
platform_reset(void)
{
  Io::write(Io::read<Mword>(Tegra2_reset::RESET) | 4, Tegra2_reset::RESET);
  for (;;)
    ;
}
