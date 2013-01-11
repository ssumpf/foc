INTERFACE [arm && realview]: // -------------------------------------------

#include "globalconfig.h"

EXTENSION class Mem_layout
{
public:
  enum Phys_layout_realview_all {
    Sdram_phys_base      = CONFIG_PF_REALVIEW_RAM_PHYS_BASE,
    Flush_area_phys_base = 0xe0000000,

    Devices0_map_base      = Registers_map_start,
    Devices1_map_base      = Registers_map_start + 0x00100000,
    Devices2_map_base      = Registers_map_start + 0x00200000,

  };
};

// ------------------------------------------------------------------------
INTERFACE [arm && realview && (realview_eb || realview_pb11mp || realview_pbx || realview_vexpress)]:

#include "globalconfig.h"

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview {
    System_regs_map_base = Devices0_map_base,
    System_ctrl_map_base = Devices0_map_base + 0x00001000,
    Uart0_map_base       = Devices0_map_base + 0x00009000,
    Uart1_map_base       = Devices0_map_base + 0x0000a000,
    Uart2_map_base       = Devices0_map_base + 0x0000b000,
    Uart3_map_base       = Devices0_map_base + 0x0000c000,
    Timer0_map_base      = Devices0_map_base + 0x00011000,
    Timer1_map_base      = Devices0_map_base + 0x00011020,
    Timer2_map_base      = Devices0_map_base + 0x00012000,
    Timer3_map_base      = Devices0_map_base + 0x00012020,
    Uart_base            = Uart0_map_base,
  };

  enum Phys_layout_realview {
    Devices0_phys_base   = 0x10000000,
    System_regs_phys_base= Devices0_phys_base,
    System_ctrl_phys_base= Devices0_phys_base + 0x00001000,
    Uart0_phys_base      = Devices0_phys_base + 0x00009000,
    Timer0_1_phys_base   = Devices0_phys_base + 0x00011000,
    Timer2_3_phys_base   = Devices0_phys_base + 0x00012000,
  };
};

// ------------------------------------------------------------------------
INTERFACE [arm && realview && realview_eb && !(mpcore || armca9)]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview_single {
    Gic_cpu_map_base    = Devices0_map_base  + 0x00040000,
    Gic_dist_map_base   = Gic_cpu_map_base + 0x00001000,
  };

  enum Phys_layout_realview_single {
    Gic_cpu_phys_base   = Devices0_phys_base  + 0x00040000,
    Gic_dist_phys_base  = Gic_cpu_phys_base + 0x00001000,
  };
};

// ------------------------------------------------------------------------
INTERFACE [arm && realview && realview_eb && (mpcore || armca9)]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview_mp {
    Mp_scu_map_base      = Devices1_map_base,
    Gic_cpu_map_base     = Devices1_map_base + 0x00000100,
    Gic_dist_map_base    = Devices1_map_base + 0x00001000,
    L2cxx0_map_base      = Devices1_map_base + 0x00002000,

    Gic1_cpu_map_base    = Devices0_map_base + 0x00040000,
    Gic1_dist_map_base   = Devices0_map_base + 0x00041000,
  };

  enum Phys_layout_realview_mp {
    Mp_scu_phys_base     = 0x1f000000,
    Gic_cpu_phys_base    = Mp_scu_phys_base  + 0x00000100,
    Gic_dist_phys_base   = Mp_scu_phys_base  + 0x00001000,
    L2cxx0_phys_base     = Mp_scu_phys_base  + 0x00002000,

    Gic1_cpu_phys_base   = Devices0_phys_base + 0x00040000,
    Gic1_dist_phys_base  = Devices0_phys_base + 0x00041000,
    Gic2_cpu_phys_base   = Devices0_phys_base + 0x00050000,
    Gic2_dist_phys_base  = Devices0_phys_base + 0x00051000,
    Gic3_cpu_phys_base   = Devices0_phys_base + 0x00060000,
    Gic3_dist_phys_base  = Devices0_phys_base + 0x00061000,
    Gic4_cpu_phys_base   = Devices0_phys_base + 0x00070000,
    Gic4_dist_phys_base  = Devices0_phys_base + 0x00071000,
  };
};

// ------------------------------------------------------------------------
INTERFACE [arm && realview && realview_pb11mp]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview_pb11mp {
    Mp_scu_map_base      = Devices1_map_base,
    Gic_cpu_map_base     = Devices1_map_base + 0x00000100,
    Gic_dist_map_base    = Devices1_map_base + 0x00001000,
    L2cxx0_map_base      = Devices1_map_base + 0x00002000,

    Gic1_cpu_map_base    = Devices2_map_base,
    Gic1_dist_map_base   = Devices2_map_base + 0x00001000,
  };

  enum Phys_layout_realview_pb11mp {
    Devices1_phys_base   = 0x1f000000,
    Mp_scu_phys_base     = 0x1f000000,
    Gic_cpu_phys_base    = Mp_scu_phys_base + 0x00000100,
    Gic_dist_phys_base   = Mp_scu_phys_base + 0x00001000,
    L2cxx0_phys_base     = Mp_scu_phys_base + 0x00002000,

    Devices2_phys_base   = 0x1e000000,
    Gic0_cpu_phys_base   = 0x1e000000,
    Gic0_dist_phys_base  = Gic0_cpu_phys_base + 0x00001000,
    Gic1_cpu_phys_base   = 0x1e010000,
    Gic1_dist_phys_base  = Gic1_cpu_phys_base + 0x00001000,
  };
};

// ------------------------------------------------------------------------
INTERFACE [arm && realview && realview_pbx]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview_pbx {
    Mp_scu_map_base      = Devices1_map_base,
    Gic_cpu_map_base     = Devices1_map_base + 0x00000100,
    Gic_dist_map_base    = Devices1_map_base + 0x00001000,
    L2cxx0_map_base      = Devices1_map_base + 0x00002000,

    Gic2_cpu_map_base    = Devices2_map_base + 0x00020000,
    Gic2_dist_map_base   = Devices2_map_base + 0x00021000,
    Gic3_cpu_map_base    = Devices2_map_base + 0x00030000,
    Gic3_dist_map_base   = Devices2_map_base + 0x00031000,
  };

  enum Phys_layout_realview_pbx {
    Devices1_phys_base   = 0x1f000000,
    Mp_scu_phys_base     = 0x1f000000,
    Gic_cpu_phys_base    = Mp_scu_phys_base + 0x00000100,
    Gic_dist_phys_base   = Mp_scu_phys_base + 0x00001000,
    L2cxx0_phys_base     = Mp_scu_phys_base + 0x00002000,

    Devices2_phys_base   = 0x1e000000,
    Gic2_cpu_phys_base   = 0x1e020000,
    Gic2_dist_phys_base  = Gic2_cpu_phys_base + 0x00001000,
    Gic3_cpu_phys_base   = 0x1e030000,
    Gic3_dist_phys_base  = Gic3_cpu_phys_base + 0x00001000,
  };
};

// ------------------------------------------------------------------------
INTERFACE [arm && realview && realview_vexpress]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview_vexpress {
    Mp_scu_map_base      = Devices1_map_base,
    Gic_cpu_map_base     = Devices1_map_base + 0x00000100,
    Gic_dist_map_base    = Devices1_map_base + 0x00001000,
    L2cxx0_map_base      = Devices1_map_base + 0x00002000,
  };

  enum Phys_layout_realview_vexpress {
    Devices1_phys_base   = 0x1e000000,
    Mp_scu_phys_base     = 0x1e000000,
    Gic_cpu_phys_base    = Mp_scu_phys_base + 0x00000100,
    Gic_dist_phys_base   = Mp_scu_phys_base + 0x00001000,
    L2cxx0_phys_base     = Mp_scu_phys_base + 0x00002000,
  };
};
