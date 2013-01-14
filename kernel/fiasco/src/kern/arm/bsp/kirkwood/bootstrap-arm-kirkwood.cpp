INTERFACE [arm && kirkwood]:

#include "mem_layout.h"

enum { Cache_flush_area = 0, };

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && kirkwood]:

void
map_hw(void *pd)
{
  map_dev<Mem_layout::Devices0_phys_base>(pd, 0);
}
