//-----------------------------------------------------------------------------
INTERFACE [arm && pxa]:

enum {
  Cache_flush_area = 0xa0100000, // XXX: hacky
};

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && pxa]:

void
map_hw(void *pd)
{
  // map the cache flush area to 0xef000000
  map_1mb(pd, Mem_layout::Cache_flush_area, Mem_layout::Flush_area_phys_base, true, false);

  map_dev<Mem_layout::Devices0_phys_base>(pd, 0);
  map_dev<Mem_layout::Devices1_phys_base>(pd, 1);
  map_dev<Mem_layout::Devices2_phys_base>(pd, 2);
}
