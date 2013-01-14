//---------------------------------------------------------------------------
INTERFACE[arm && sa1100]:

enum {
  Cache_flush_area = 0xe0000000,
};

//---------------------------------------------------------------------------
IMPLEMENTATION[arm && sa1100]:

#include "mem_layout.h"

void
map_hw(void *pd)
{
  // map the cache flush area to 0xef000000
  map_1mb(pd, Mem_layout::Cache_flush_area, Mem_layout::Flush_area_phys_base, true, false);

  map_dev<Mem_layout::Devices0_phys_base>(pd, 0);
  map_dev<Mem_layout::Devices1_phys_base>(pd, 1);
}
