//-----------------------------------------------------------------------------
INTERFACE [arm && s3c2410]:

enum {
  Cache_flush_area = 0x0,
};


//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && s3c2410]:

void
map_hw(void *pd)
{
  // map UART
  map_1mb(pd, Mem_layout::Uart_map_base, Mem_layout::Uart_phys_base, false, false);
  // map Timer
  map_1mb(pd, Mem_layout::Timer_map_base, Mem_layout::Timer_phys_base, false, false);
  // map Pic
  map_1mb(pd, Mem_layout::Pic_map_base, Mem_layout::Pic_phys_base, false, false);

  // map watchdog
  map_1mb(pd, Mem_layout::Watchdog_map_base, Mem_layout::Watchdog_phys_base, false, false);
}
