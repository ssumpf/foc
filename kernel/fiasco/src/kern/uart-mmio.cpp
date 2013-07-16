IMPLEMENTATION [libuart]:

#include "io_regblock.h"
#include "kmem.h"

EXTENSION class Uart
{
  L4::Io_register_block_mmio _regs;
};

IMPLEMENT inline NEEDS["kmem.h"]
Uart::Uart() : _regs(Kmem::mmio_remap(base())) {}

PUBLIC bool Uart::startup()
{ return uart()->startup(&_regs); }

