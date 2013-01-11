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
  // map UART
  map_1mb(pd, Mem_layout::Uart_map_base, Mem_layout::Uart_phys_base, false, false);
  map_1mb(pd, Mem_layout::Uart_phys_base, Mem_layout::Uart_phys_base, false, false);
  // map Timer
  map_1mb(pd, Mem_layout::Timer_map_base, Mem_layout::Timer_phys_base, false, false);
  // map Pic
  map_1mb(pd, Mem_layout::Pic_map_base, Mem_layout::Pic_phys_base, false, false);
}
