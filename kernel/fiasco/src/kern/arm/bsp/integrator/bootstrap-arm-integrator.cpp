//-----------------------------------------------------------------------------
INTERFACE [arm && integrator]:

enum {
  Cache_flush_area = 0xe0000000,
};

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && integrator]:
void
map_hw(void *pd)
{
  // map UART
  map_1mb(pd, Mem_layout::Uart_map_base, Mem_layout::Uart_phys_base, false, false);
  // map Timer
  map_1mb(pd, Mem_layout::Timer_map_base, Mem_layout::Timer_phys_base, false, false);
  // map Pic
  map_1mb(pd, Mem_layout::Pic_map_base, Mem_layout::Pic_phys_base, false, false);
  // map Integrator hdr
  map_1mb(pd, Mem_layout::Integrator_map_base, Mem_layout::Integrator_phys_base, false, false);
}
