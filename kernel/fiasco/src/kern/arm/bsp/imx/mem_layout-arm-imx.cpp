INTERFACE [arm && imx]: //----------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Phys_layout_imx : Address {
    Flush_area_phys_base = 0xe0000000,
  };
};

INTERFACE [arm && imx && imx21]: // ---------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Phys_layout_imx21 : Address {
    Uart_phys_base        = 0x1000a000,
    Timer_phys_base       = 0x10003000,
    Pll_phys_base         = 0x10027000,
    Watchdog_phys_base    = 0x10002000,
    Pic_phys_base         = 0x10040000,
  };
};

INTERFACE [arm && imx && imx35]: // ---------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Phys_layout_imx35 : Address {
    Uart_phys_base        = 0x43f90000, // uart1
    Timer_phys_base       = 0x53f94000, // epit1
    Watchdog_phys_base    = 0x53fdc000, // wdog
    Pic_phys_base         = 0x68000000,
  };
};


INTERFACE [arm && imx && imx51]: // ---------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Phys_layout_imx51 : Address {
    Timer_phys_base       = 0x73fac000, // epit1
    Uart_phys_base        = 0x73fbc000, // uart1
    Watchdog_phys_base    = 0x73f98000, // wdog1
    Gic_dist_phys_base    = 0xe0000000,
    Gic_cpu_phys_base     = 0xe0000000, // this is a fake address and not used
  };
};

INTERFACE [arm && imx && imx53]: // ---------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Phys_layout_imx53 : Address {
    Timer_phys_base       = 0x53fac000, // epit1
    Uart_phys_base        = 0x53fbc000, // uart1
    Watchdog_phys_base    = 0x53f98000, // wdog1
    Gic_dist_phys_base    = 0x0fffc000,
    Gic_cpu_phys_base     = 0x0fffc000, // this is a fake address and not used
  };
};

INTERFACE [arm && imx && imx6]: // -----------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Phys_layout_imx6 : Address {
    Mp_scu_phys_base     = 0x00a00000,
    Gic_cpu_phys_base    = 0x00a00100,
    Gic_dist_phys_base   = 0x00a01000,
    L2cxx0_phys_base     = 0x00a02000,

    Watchdog_phys_base   = 0x020bc000, // wdog1
    Gpt_phys_base        = 0x02098000,
    Src_phys_base        = 0x020d8000,
    Uart1_phys_base      = 0x02020000,
    Uart2_phys_base      = 0x021e8000,
    Uart_phys_base       = CONFIG_PF_IMX_UART_NR == 2
                           ? Uart2_phys_base : Uart1_phys_base,
  };
};
