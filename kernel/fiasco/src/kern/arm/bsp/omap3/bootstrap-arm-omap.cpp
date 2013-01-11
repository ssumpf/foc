INTERFACE [arm && omap]:

enum {
  Cache_flush_area = 0,
};

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && omap]:

#include "mem_layout.h"
#include "io.h"

void
map_hw(void *pd)
{
  // map devices
  map_1mb(pd, Mem_layout::Devices1_map_base, Mem_layout::Devices1_phys_base, false, false);
  map_1mb(pd, Mem_layout::Devices2_map_base, Mem_layout::Devices2_phys_base, false, false);
  map_1mb(pd, Mem_layout::Devices3_map_base, Mem_layout::Devices3_phys_base, false, false);
  map_1mb(pd, Mem_layout::Devices4_map_base, Mem_layout::Devices4_phys_base, false, false);
}
