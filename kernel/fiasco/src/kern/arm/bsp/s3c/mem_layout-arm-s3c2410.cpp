INTERFACE [arm && s3c2410]: //----------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_s3c2410 {
    Uart_map_base        = 0xef100000,
    Timer_map_base       = 0xef200000,
    Pic_map_base         = 0xef300000,
    Watchdog_map_base    = 0xef400000,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout {
    Uart_phys_base       = 0x50000000,
    Timer_phys_base      = 0x51000000,
    Pic_phys_base        = 0x4a000000,
    Watchdog_phys_base   = 0x53000000,
    Sdram_phys_base      = 0x30000000,
    Flush_area_phys_base = 0xe0000000,
  };
};
