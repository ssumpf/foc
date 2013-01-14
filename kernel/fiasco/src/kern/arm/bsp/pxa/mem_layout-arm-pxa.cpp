INTERFACE [arm-pxa]: //------------------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_pxa : Address {
    Timer_map_base       = Devices0_map_base,
    Pic_map_base         = Devices1_map_base,
    Uart_map_base        = Devices2_map_base,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout : Address {
    Devices0_phys_base   = 0x40a00000,
    Devices1_phys_base   = 0x40d00000,
    Devices2_phys_base   = 0x40100000,
    Sdram_phys_base      = 0xa0000000,
    Flush_area_phys_base = 0xe0000000,
  };
};

