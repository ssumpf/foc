INTERFACE [ppc32 && qemu]:

#include "types.h"

class Irq_base;

EXTENSION class Pic
{
public:
  enum { No_irq_pending = ~0U };
};

//------------------------------------------------------------------------------
IMPLEMENTATION [ppc32 && qemu]:

#include "initcalls.h"

IMPLEMENT FIASCO_INIT
void
Pic::init()
{
}

PUBLIC static inline
Unsigned32
Pic::pending()
{
  return 0;
}

IMPLEMENT inline
void
Pic::block_locked (unsigned)
{}

IMPLEMENT inline
void
Pic::acknowledge_locked(unsigned)
{}

IMPLEMENT inline
void
Pic::enable_locked (unsigned, unsigned)
{}

IMPLEMENT inline
void
Pic::disable_locked (unsigned)
{}
