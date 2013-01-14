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
  map_dev<Mem_layout::Devices1_phys_base>(pd, 1);
  map_dev<Mem_layout::Devices2_phys_base>(pd, 2);
  map_dev<Mem_layout::Devices3_phys_base>(pd, 3);
  map_dev<Mem_layout::Devices4_phys_base>(pd, 4);
}
