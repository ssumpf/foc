IMPLEMENTATION [arm && kirkwood]:

#include "io.h"
#include "kmem.h"

class Kirkwood_reset
{
public:
  enum
  {
    Mask_reg       = Mem_layout::Reset_map_base + 0x20108,
    Soft_reset_reg = Mem_layout::Reset_map_base + 0x2010c,
  };
};

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && kirkwood]:

void __attribute__ ((noreturn))
platform_reset(void)
{
  // enable software reset
  Io::write(1 << 2, Kirkwood_reset::Mask_reg);

  // do software reset
  Io::write(1, Kirkwood_reset::Soft_reset_reg);

  for (;;)
    ;
}
