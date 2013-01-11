//---------------------------------------------------------------------------
INTERFACE [arm-sa1100]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_sa1100 {
    Uart_map_base        = 0xef100000,
    Timer_map_base       = 0xef200000,
    Pic_map_base         = 0xef250000,
    Uart_base            = Uart_map_base + 0x50000,
  };

  enum Phys_layout {
    Uart_phys_base       = 0x80050000,
    Timer_phys_base      = 0x90000000,
    Pic_phys_base        = 0x90050000,
    Sdram_phys_base      = 0xc0000000,
    Flush_area_phys_base = 0xe0000000,
  };
};


