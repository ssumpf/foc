INTERFACE [arm-integrator]: //----------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_integrator {
    Uart_map_base        = 0xef100000,
    Timer_map_base       = 0xef200000,
    Pic_map_base         = 0xef300000,
    Integrator_map_base  = 0xef400000,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout {
    Uart_phys_base       = 0x16000000,
    Timer_phys_base      = 0x13000000,
    Pic_phys_base        = 0x14000000,
    Integrator_phys_base = 0x10000000,
    Sdram_phys_base      = 0x00000000,
    Flush_area_phys_base = 0xe0000000,
  };
};

