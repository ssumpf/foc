INTERFACE [arm && realview]:

#include "kmem.h"
#include "l4_types.h"

EXTENSION class Clock_base
{
protected:
  enum {
    SYS_24MHZ = Kmem::System_regs_map_base + 0x5c,
  };

  typedef Mword Counter;
};

// --------------------------------------------------------------
IMPLEMENTATION [arm && realview]:

#include "io.h"
#include <cstdio>

IMPLEMENT inline NEEDS["io.h", <cstdio>]
Clock::Counter
Clock::read_counter() const
{
  return Io::read<Mword>(SYS_24MHZ);
}

IMPLEMENT inline
Cpu_time
Clock::us(Time t)
{
  return t / 24;
}
