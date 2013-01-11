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
  // map UART
  map_1mb(pd, Mem_layout::Uart_map_base, Mem_layout::Uart_phys_base, false, false);
  map_1mb(pd, Mem_layout::Uart_phys_base, Mem_layout::Uart_phys_base, false, false);
  // map Timer and Pic
  map_1mb(pd, Mem_layout::Timer_map_base, Mem_layout::Timer_phys_base, false, false);
}
