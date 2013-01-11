INTERFACE [arm && tegra2]:

#include "mem_layout.h"

enum { Cache_flush_area = 0, };

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && tegra2]:

void
map_hw(void *pd)
{
  map_1mb(pd, Mem_layout::Devices0_map_base, Mem_layout::Devices0_phys_base, false, false);
  map_1mb(pd, Mem_layout::Devices1_map_base, Mem_layout::Devices1_phys_base, false, false);
  map_1mb(pd, Mem_layout::Devices2_map_base, Mem_layout::Devices2_phys_base, false, false);
}
