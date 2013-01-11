IMPLEMENTATION [arm && realview]:

#include "io.h"
#include "kmem.h"

class Realview_reset
{
public:
  enum
  {
    LOCK  = Kmem::System_regs_map_base + 0x20,
    RESET = Kmem::System_regs_map_base + 0x40,
  };
};

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && realview && realview_eb]:

static inline void do_reset()
{
  Io::write(0x108, Realview_reset::RESET); // the 0x100 is for Qemu
}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && realview && realview_pb11mp]:

static inline void do_reset()
{
  Io::write(0x4, Realview_reset::RESET); // PORESET (0x8 would also be ok)
}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && realview && (realview_pbx || realview_vexpress)]:

static inline void do_reset()
{
  Io::write(0x104, Realview_reset::RESET); // POWER reset, 0x100 for Qemu
}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && realview]:

void __attribute__ ((noreturn))
platform_reset(void)
{
  Io::write(0xa05f, Realview_reset::LOCK);  // unlock for reset
  do_reset();

  for (;;)
    ;
}
