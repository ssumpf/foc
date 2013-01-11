INTERFACE [arm && imx]: //----------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_imx {
    Device_map_base_1    = Registers_map_start,
    Device_map_base_2    = Registers_map_start + 0x100000,
    Device_map_base_3    = Registers_map_start + 0x200000,
  };

  enum Phys_layout {
    Sdram_phys_base      = CONFIG_PF_IMX_RAM_PHYS_BASE,
    Flush_area_phys_base = 0xe0000000,
  };
};

INTERFACE [arm && imx && imx21]: // ---------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_imx21 {
    Uart_map_base        = 0xef10a000,
    Timer_map_base       = 0xef103000,
    Pll_map_base         = 0xef127000,
    Watchdog_map_base    = 0xef102000,
    Pic_map_base         = 0xef140000,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout_imx21 {
    Device_phys_base_1   = 0x10000000,

    Timer_phys_base      = 0x10003000,
    Uart_phys_base       = 0x1000a000,
    Pll_phys_base        = 0x10027000,
    Watchdog_phys_base   = 0x10002000,
    Pic_phys_base        = 0x10040000,
  };
};

INTERFACE [arm && imx && imx35]: // ---------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_imx35 {
    Uart_map_base        = Device_map_base_1 + 0x90000,
    Timer_map_base       = Device_map_base_2 + 0x94000,
    Watchdog_map_base    = Device_map_base_2 + 0xdc000,
    Pic_map_base         = Device_map_base_3 + 0x0,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout_imx35 {
    Device_phys_base_1   = 0x43f00000,
    Device_phys_base_2   = 0x53f00000,
    Device_phys_base_3   = 0x68000000,

    Timer_phys_base      = 0x53f94000, // epit1
    Uart_phys_base       = 0x43f90000, // uart1
    Watchdog_phys_base   = 0x53fdc000, // wdog
    Pic_phys_base        = 0x68000000,
  };
};


INTERFACE [arm && imx && imx51]: // ---------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_imx51 {
    Timer_map_base       = 0xef1ac000,
    Uart_map_base        = 0xef1bc000,
    Watchdog_map_base    = 0xef198000,
    Gic_cpu_map_base     = 0,
    Gic_dist_map_base    = 0xef200000,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout_imx51 {
    Device_phys_base_1   = 0x73f00000,
    Device_phys_base_2   = 0xe0000000,

    Watchdog_phys_base   = 0x73f98000, // wdog1
    Timer_phys_base      = 0x73fac000, // epit1
    Uart_phys_base       = 0x73fbc000, // uart1
    Gic_dist_phys_base   = 0xe0000000,
  };
};
