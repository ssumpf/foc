INTERFACE [arm && realview]: // -------------------------------------------

#include "globalconfig.h"

EXTENSION class Mem_layout
{
public:
  enum Phys_layout_realview_all : Address {
    Sdram_phys_base      = CONFIG_PF_REALVIEW_RAM_PHYS_BASE,
    Flush_area_phys_base = 0xe0000000,
  };
};

// ------------------------------------------------------------------------
INTERFACE [arm && realview && (realview_eb || realview_pb11mp || realview_pbx || realview_vexpress)]:

#include "globalconfig.h"

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview : Address {
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

  enum Phys_layout_realview : Address {
    Devices0_phys_base   = 0x10000000,
  };
};

// ------------------------------------------------------------------------
INTERFACE [arm && realview && realview_eb && !(mpcore || armca9)]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview_single : Address {
    Gic_cpu_map_base    = Devices0_map_base  + 0x00040000,
    Gic_dist_map_base   = Gic_cpu_map_base   + 0x00001000,
  };

  enum Phys_layout_realview_single : Address {
    Devices1_phys_base   = Invalid_address,
    Devices2_phys_base   = Invalid_address,
  };
};

// ------------------------------------------------------------------------
INTERFACE [arm && realview && realview_eb && (mpcore || armca9)]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview_mp : Address {
    Mp_scu_map_base      = Devices1_map_base,
    Gic_cpu_map_base     = Devices1_map_base + 0x00000100,
    Gic_dist_map_base    = Devices1_map_base + 0x00001000,
    L2cxx0_map_base      = Devices1_map_base + 0x00002000,

    Gic1_cpu_map_base    = Devices0_map_base + 0x00040000,
    Gic1_dist_map_base   = Devices0_map_base + 0x00041000,

    Gic2_cpu_map_base    = Devices0_map_base + 0x00050000,
    Gic2_dist_map_base   = Devices0_map_base + 0x00051000,
    Gic3_cpu_map_base    = Devices0_map_base + 0x00060000,
    Gic3_dist_map_base   = Devices0_map_base + 0x00061000,
    Gic4_cpu_map_base    = Devices0_map_base + 0x00070000,
    Gic4_dist_map_base   = Devices0_map_base + 0x00071000,
  };

  enum Phys_layout_realview_mp : Address {
    Devices1_phys_base   = 0x1f000000,
    Devices2_phys_base   = Invalid_address,
  };
};

// ------------------------------------------------------------------------
INTERFACE [arm && realview && realview_pb11mp]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview_pb11mp : Address {
    Mp_scu_map_base      = Devices1_map_base,
    Gic_cpu_map_base     = Devices1_map_base + 0x00000100,
    Gic_dist_map_base    = Devices1_map_base + 0x00001000,
    L2cxx0_map_base      = Devices1_map_base + 0x00002000,

    Gic1_cpu_map_base    = Devices2_map_base,
    Gic1_dist_map_base   = Devices2_map_base + 0x00001000,
  };

  enum Phys_layout_realview_pb11mp : Address {
    Devices1_phys_base   = 0x1f000000,
    Devices2_phys_base   = 0x1e000000,
  };
};

// ------------------------------------------------------------------------
INTERFACE [arm && realview && realview_pbx]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview_pbx : Address {
    Mp_scu_map_base      = Devices1_map_base,
    Gic_cpu_map_base     = Devices1_map_base + 0x00000100,
    Gic_dist_map_base    = Devices1_map_base + 0x00001000,
    L2cxx0_map_base      = Devices1_map_base + 0x00002000,

    Gic2_cpu_map_base    = Devices2_map_base + 0x00020000,
    Gic2_dist_map_base   = Devices2_map_base + 0x00021000,
    Gic3_cpu_map_base    = Devices2_map_base + 0x00030000,
    Gic3_dist_map_base   = Devices2_map_base + 0x00031000,
  };

  enum Phys_layout_realview_pbx : Address {
    Devices1_phys_base   = 0x1f000000,
    Devices2_phys_base   = 0x1e000000,
  };
};

// ------------------------------------------------------------------------
INTERFACE [arm && realview && realview_vexpress]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_realview_vexpress : Address {
    Mp_scu_map_base      = Devices1_map_base,
    Gic_cpu_map_base     = Devices1_map_base + 0x00000100,
    Gic_dist_map_base    = Devices1_map_base + 0x00001000,
    L2cxx0_map_base      = Devices1_map_base + 0x00002000,
  };

  enum Phys_layout_realview_vexpress : Address {
    Devices1_phys_base   = 0x1e000000,
    Devices2_phys_base   = Invalid_address,
  };
};
