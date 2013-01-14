INTERFACE [arm-integrator]: //----------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_integrator : Address {
    Uart_map_base        = Devices0_map_base,
    Timer_map_base       = Devices1_map_base,
    Pic_map_base         = Devices2_map_base,
    Integrator_map_base  = Devices3_map_base,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout : Address {
    Devices0_phys_base   = 0x16000000,
    Devices1_phys_base   = 0x13000000,
    Devices2_phys_base   = 0x14000000,
    Devices3_phys_base   = 0x10000000,
    Sdram_phys_base      = 0x00000000,
    Flush_area_phys_base = 0xe0000000,
  };
};

