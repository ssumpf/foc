INTERFACE [arm && kirkwood]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_kirkwood : Address
  {
    Uart_base            = Devices0_map_base + 0x00012000,
    Reset_map_base       = Devices0_map_base,
    Timer_map_base       = Devices0_map_base,
    Pic_map_base         = Devices0_map_base,
  };

  enum Phys_layout_kirkwood: Address
  {
    Devices0_phys_base   = 0xf1000000,
    Sdram_phys_base      = 0x0,
  };
};
