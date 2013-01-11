//-----------------------------------------------------------------------------
INTERFACE [arm && imx]:

enum {
  Cache_flush_area = 0xe0000000,
};

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && imx21]:
void
map_hw(void *pd)
{
  // map devices
  map_1mb(pd, Mem_layout::Device_map_base_1, Mem_layout::Device_phys_base_1, false, false);
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && imx35]:
void
map_hw(void *pd)
{
  map_1mb(pd, Mem_layout::Device_map_base_1, Mem_layout::Device_phys_base_1, false, false);
  map_1mb(pd, Mem_layout::Device_map_base_2, Mem_layout::Device_phys_base_2, false, false);
  map_1mb(pd, Mem_layout::Device_map_base_3, Mem_layout::Device_phys_base_3, false, false);
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && imx51]:
void
map_hw(void *pd)
{
  map_1mb(pd, Mem_layout::Device_map_base_1, Mem_layout::Device_phys_base_1, false, false);
  map_1mb(pd, Mem_layout::Device_map_base_2, Mem_layout::Device_phys_base_2, false, false);
}
