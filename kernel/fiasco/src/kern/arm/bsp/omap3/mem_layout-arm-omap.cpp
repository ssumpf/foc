INTERFACE [arm && omap3_35x]: //-------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_omap3_35x {
    Devices1_map_base       = Registers_map_start,
    L4_addr_prot_map_base   = Devices1_map_base + 0x00040000,
    Uart1_map_base          = Devices1_map_base + 0x0006a000,
    Gptimer10_map_base      = Devices1_map_base + 0x00086000,
    Wkup_cm_map_base        = Devices1_map_base + 0x00004c00,

    Devices2_map_base       = Registers_map_start + 0x00100000,
    Intc_map_base           = Devices2_map_base   + 0x0,

    Devices3_map_base       = Registers_map_start + 0x00200000,
    Timer1ms_map_base       = Devices3_map_base   + 0x00018000,
    Prm_global_reg_map_base = Devices3_map_base   + 0x00007200,

    Devices4_map_base       = Registers_map_start + 0x00300000,
    Uart3_map_base          = Devices4_map_base   + 0x00020000,
  };

  enum Phys_layout_omap3_35x {
    Devices1_phys_base       = 0x48000000,
    Devices2_phys_base       = 0x48200000,
    Devices3_phys_base       = 0x48300000,
    Devices4_phys_base       = 0x49000000,
    Sdram_phys_base          = 0x80000000,
    Flush_area_phys_base     = 0xe0000000,
  };
};

INTERFACE [arm && omap3_35xevm]: //----------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_omap3_35xevm {
    Uart_base               = Uart1_map_base,
  };
};

INTERFACE [arm && omap3_beagleboard]: //-----------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_omap3_beagleboard {
    Uart_base               = Uart3_map_base,
  };
};

INTERFACE [arm && omap3_am33xx]: //----------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_omap3_335x {
    Devices1_map_base       = Registers_map_start,
    Devices2_map_base       = Registers_map_start + 0x00100000,
    Devices3_map_base       = Registers_map_start + 0x00200000,
    Devices4_map_base       = Registers_map_start + 0x00300000,

    Cm_per_map_base         = Devices1_map_base + 0x00000000,
    Cm_wkup_map_base        = Devices1_map_base + 0x00000400,
    Cm_dpll_map_base        = Devices1_map_base + 0x00000500,
    Timergen_map_base       = Devices1_map_base + 0x00005000, // DMTIMER0
    Timer1ms_map_base       = Devices1_map_base + 0x00031000,
    Uart1_map_base          = Devices1_map_base + 0x00009000,
    Prm_global_reg_map_base = Devices3_map_base   + 0x00007200,
    Intc_map_base           = Devices4_map_base   + 0x0,
    Uart_base               = Uart1_map_base,
  };

  enum Phys_layout_omap3_335x {
    Devices1_phys_base       = 0x44e00000,
    Devices2_phys_base       = 0x48000000,
    Devices3_phys_base       = 0x48100000,
    Devices4_phys_base       = 0x48200000,
    Sdram_phys_base          = 0x80000000,
    Flush_area_phys_base     = 0xe0000000,
  };
};

INTERFACE [arm && omap4]: //-----------------------------------------------

EXTENSION class Mem_layout
{
public:
  enum Phys_layout_omap4 {
    Devices1_phys_base       = 0x48000000,
    Devices2_phys_base       = 0x48200000,
    Devices3_phys_base       = 0x4a300000,
    Devices4_phys_base       = 0x49000000,
    Sdram_phys_base          = 0x80000000,
  };

  enum Virt_layout_omap4_pandaboard {
    Devices1_map_base       = Registers_map_start,
    Devices2_map_base       = Registers_map_start + 0x00100000,
    Devices3_map_base       = Registers_map_start + 0x00200000,
    Devices4_map_base       = Registers_map_start + 0x00300000,

    Uart_base               = Devices1_map_base + 0x20000,
    Mp_scu_map_base         = Devices2_map_base + 0x40000,
    Gic_cpu_map_base        = Devices2_map_base + 0x40100,
    Gic_dist_map_base       = Devices2_map_base + 0x41000,

    __Timer = Devices2_map_base + 0x40600,
    __PL310 = Devices2_map_base + 0x42000,

    Prm_map_base            = Devices3_map_base + 0x6000,
  };
};
