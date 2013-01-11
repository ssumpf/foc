INTERFACE [arm && kirkwood]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_kirkwood
  {
    Devices0_map_base    = Registers_map_start,
    Devices1_map_base    = Registers_map_start + 0x00100000,
    Devices2_map_base    = Registers_map_start + 0x00200000,

    Uart_base            = Devices0_map_base + 0x00012000,
    Reset_map_base       = Devices0_map_base,
    Timer_map_base       = Devices0_map_base,
    Pic_map_base         = Devices0_map_base,
  };

  enum Phys_layout_kirkwood
  {
    Devices0_phys_base   = 0xf1000000,
    Sdram_phys_base      = 0x0,
  };
};
