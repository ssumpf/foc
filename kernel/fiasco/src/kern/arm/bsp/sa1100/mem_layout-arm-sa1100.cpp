//---------------------------------------------------------------------------
INTERFACE [arm-sa1100]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_sa1100 : Address {
    Uart_base            = Devices0_map_base + 0x50000,
    Timer_map_base       = Devices1_map_base,
    Pic_map_base         = Devices1_map_base + 0x50000,
  };

  enum Phys_layout : Address {
    Devices0_phys_base   = 0x80000000,
    Devices1_phys_base   = 0x90000000,
    Sdram_phys_base      = 0xc0000000,
    Flush_area_phys_base = 0xe0000000,
  };
};
