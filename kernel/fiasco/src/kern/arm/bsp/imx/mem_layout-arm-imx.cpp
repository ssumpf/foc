INTERFACE [arm && imx]: //----------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Phys_layout : Address {
    Sdram_phys_base      = CONFIG_PF_IMX_RAM_PHYS_BASE,
    Flush_area_phys_base = 0xe0000000,
  };
};

INTERFACE [arm && imx && imx21]: // ---------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_imx21 : Address {
    Uart_map_base        = Devices1_map_base + 0x0a000,
    Timer_map_base       = Devices1_map_base + 0x03000,
    Pll_map_base         = Devices1_map_base + 0x27000,
    Watchdog_map_base    = Devices1_map_base + 0x02000,
    Pic_map_base         = Devices1_map_base + 0x40000,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout_imx21 : Address {
    Devices1_phys_base   = 0x10000000,
    Devices2_phys_base   = Invalid_address,
    Devices3_phys_base   = Invalid_address,
  };
};

INTERFACE [arm && imx && imx35]: // ---------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_imx35 : Address {
    Uart_map_base        = Devices1_map_base + 0x90000, // uart1
    Timer_map_base       = Devices2_map_base + 0x94000, // epit1
    Watchdog_map_base    = Devices2_map_base + 0xdc000, // wdog
    Pic_map_base         = Devices3_map_base + 0x0,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout_imx35 : Address {
    Devices1_phys_base   = 0x43f00000,
    Devices2_phys_base   = 0x53f00000,
    Devices3_phys_base   = 0x68000000,
  };
};


INTERFACE [arm && imx && imx51]: // ---------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_imx51 : Address {
    Timer_map_base       = Devices1_map_base + 0xac000, // epit1
    Uart_map_base        = Devices1_map_base + 0xbc000, // uart1
    Watchdog_map_base    = Devices1_map_base + 0x98000, // wdog1
    Gic_cpu_map_base     = Invalid_address,
    Gic_dist_map_base    = Devices2_map_base + 0x00000,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout_imx51 : Address {
    Devices1_phys_base   = 0x73f00000,
    Devices2_phys_base   = 0xe0000000,
    Devices3_phys_base   = Invalid_address,
  };
};

INTERFACE [arm && imx && imx53]: // ---------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_imx53 : Address {
    Timer_map_base       = Devices1_map_base + 0xac000, // epit1
    Uart_map_base        = Devices1_map_base + 0xbc000, // uart1
    Watchdog_map_base    = Devices1_map_base + 0x98000, // wdog1
    Gic_cpu_map_base     = Invalid_address,
    Gic_dist_map_base    = Devices2_map_base + 0xfc000,
    Uart_base            = Uart_map_base,
  };

  enum Phys_layout_imx53 : Address {
    Devices1_phys_base   = 0x53f00000,
    Devices2_phys_base   = 0x0ff00000,
    Devices3_phys_base   = Invalid_address,
  };
};

INTERFACE [arm && imx && imx6]: // -----------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_imx6 : Address {
    Mp_scu_map_base      = Devices1_map_base,
    Gic_cpu_map_base     = Mp_scu_map_base + 0x00100,
    Gic_dist_map_base    = Mp_scu_map_base + 0x01000,
    L2cxx0_map_base      = Mp_scu_map_base + 0x02000,

    Uart1_map_base       = Devices2_map_base + 0x20000, // uart1
    Uart2_map_base       = Devices3_map_base + 0xe8000, // uart2
    Watchdog_map_base    = Devices2_map_base + 0xbc000, // wdog1
    Gpt_map_base         = Devices2_map_base + 0x98000,
    Src_map_base         = Devices2_map_base + 0xd8000,
    Uart_base            = Uart2_map_base,
  };

  enum Phys_layout_imx6 : Address {
    Devices1_phys_base   = 0x00a00000,
    Devices2_phys_base   = 0x02000000,
    Devices3_phys_base   = 0x02100000,
  };
};
