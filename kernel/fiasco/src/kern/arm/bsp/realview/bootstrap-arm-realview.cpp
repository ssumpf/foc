INTERFACE [arm && realview]:

#include "mem_layout.h"

enum { Cache_flush_area = 0, };

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && realview && realview_eb && !(mpcore || (armca9 && mp))]:

static void map_hw2(void *)
{}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && realview && realview_eb && (mpcore || (armca9 && mp))]:

static void map_hw2(void *pd)
{
  map_1mb(pd, Mem_layout::Mp_scu_map_base, Mem_layout::Mp_scu_phys_base,
          false, false);
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && realview && (realview_pb11mp || realview_pbx)]:

static void map_hw2(void *pd)
{
  map_1mb(pd, Mem_layout::Devices1_map_base, Mem_layout::Devices1_phys_base,
          false, false);
  map_1mb(pd, Mem_layout::Devices2_map_base, Mem_layout::Devices2_phys_base,
          false, false);
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && realview && realview_vexpress]:

static void map_hw2(void *pd)
{
  map_1mb(pd, Mem_layout::Devices1_map_base, Mem_layout::Devices1_phys_base,
          false, false);
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && realview]:

void
map_hw(void *pd)
{
  // map devices
  map_1mb(pd, Mem_layout::Devices0_map_base, Mem_layout::Devices0_phys_base, false, false);
  map_hw2(pd);
}
