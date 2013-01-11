INTERFACE [arm-pxa]: //------------------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_pxa {
    Timer_map_base       = 0xef100000,
    Pic_map_base         = 0xef200000,
    Uart_map_base        = 0xef300000,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout {
    Timer_phys_base      = 0x40a00000,
    Pic_phys_base        = 0x40d00000,
    Uart_phys_base       = 0x40100000,
    Sdram_phys_base      = 0xa0000000,
    Flush_area_phys_base = 0xe0000000,
  };
};

