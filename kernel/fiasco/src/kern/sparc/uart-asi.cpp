IMPLEMENTATION [libuart]:

#include "io_regblock_asi.h"

EXTENSION class Uart
{
  L4::Io_register_block_asi _regs;
};

IMPLEMENT inline Uart::Uart() : _regs(base()) {}

PUBLIC bool Uart::startup()
{ return uart()->startup(&_regs); }


