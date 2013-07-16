INTERFACE [arm && mp && imx6]:

#include "types.h"

IMPLEMENTATION [arm && mp && imx6]:

#include "ipi.h"
#include "mem_layout.h"
#include "mmio_register_block.h"
#include "kmem.h"

PUBLIC static
void
Platform_control::boot_ap_cpus(Address phys_tramp_mp_addr)
{
  Mmio_register_block src(Kmem::mmio_remap(Mem_layout::Src_phys_base));
  enum
  {
    SRC_SCR  = 0,
    SRC_GPR1 = 0x20,
    SRC_GPR3 = 0x28,
    SRC_GPR5 = 0x30,
    SRC_GPR7 = 0x38,

    SRC_SCR_CORE1_3_ENABLE = 7 << 22,
    SRC_SCR_CORE1_3_RESET  = 7 << 14,
  };

  src.write<Mword>(phys_tramp_mp_addr, SRC_GPR3);
  src.write<Mword>(phys_tramp_mp_addr, SRC_GPR5);
  src.write<Mword>(phys_tramp_mp_addr, SRC_GPR7);

  src.modify<Mword>(SRC_SCR_CORE1_3_RESET, 0, SRC_SCR);
  src.modify<Mword>(SRC_SCR_CORE1_3_ENABLE, 0, SRC_SCR);
}
