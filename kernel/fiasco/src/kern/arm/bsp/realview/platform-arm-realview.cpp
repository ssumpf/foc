INTERFACE[arm && realview]:

#include "mem_layout.h"

class Platform
{
public:
  class Sys
  {
  public:
    enum Registers
    {
      Id        = Mem_layout::System_regs_map_base + 0x0,
      Sw        = Mem_layout::System_regs_map_base + 0x4,
      Led       = Mem_layout::System_regs_map_base + 0x8,
      Lock      = Mem_layout::System_regs_map_base + 0x20,
      Flags     = Mem_layout::System_regs_map_base + 0x30,
      Flags_clr = Mem_layout::System_regs_map_base + 0x34,
      Cnt_24mhz = Mem_layout::System_regs_map_base + 0x5c,
      Pld_ctrl1 = Mem_layout::System_regs_map_base + 0x74,
      Pld_ctrl2 = Mem_layout::System_regs_map_base + 0x78,
    };
  };

};

IMPLEMENTATION[arm && realview]:

#include "io.h"

PUBLIC static inline NEEDS["io.h"]
void
Platform::write(enum Sys::Registers reg, Mword val)
{ Io::write<Mword>(val, reg); }

PUBLIC static inline NEEDS["io.h"]
Mword
Platform::read(enum Sys::Registers reg)
{ return Io::read<Mword>(reg); }
