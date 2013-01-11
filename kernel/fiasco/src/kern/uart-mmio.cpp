IMPLEMENTATION [libuart]:

#include "io_regblock.h"

EXTENSION class Uart
{
  L4::Io_register_block_mmio _regs;
};

IMPLEMENT inline Uart::Uart() : _regs(base()) {}

PUBLIC bool Uart::startup()
{ return uart()->startup(&_regs); }

