INTERFACE [arm && mp && imx6]:

#include "types.h"

IMPLEMENTATION [arm && mp && imx6]:

#include "io.h"
#include "ipi.h"
#include "mem_layout.h"

PUBLIC static
void
Platform_control::boot_ap_cpus(Address phys_tramp_mp_addr)
{
  enum
  {
    SRC_SCR  = Mem_layout::Src_map_base + 0,
    SRC_GPR1 = Mem_layout::Src_map_base + 0x20,
    SRC_GPR3 = Mem_layout::Src_map_base + 0x28,
    SRC_GPR5 = Mem_layout::Src_map_base + 0x30,
    SRC_GPR7 = Mem_layout::Src_map_base + 0x38,

    SRC_SCR_CORE1_3_ENABLE = 7 << 22,
    SRC_SCR_CORE1_3_RESET  = 7 << 14,
  };

  Io::write<Mword>(phys_tramp_mp_addr, SRC_GPR3);
  Io::write<Mword>(phys_tramp_mp_addr, SRC_GPR5);
  Io::write<Mword>(phys_tramp_mp_addr, SRC_GPR7);

  Io::set<Mword>(SRC_SCR_CORE1_3_RESET, SRC_SCR);
  Io::set<Mword>(SRC_SCR_CORE1_3_ENABLE, SRC_SCR);
}
