INTERFACE [arm && exynos5]:

EXTENSION class Mem_layout
{
public:
  enum  Phys_layout_exynos5 : Address {
    Devices1_phys_base = 0x10000000,
    Devices2_phys_base = 0x12c00000,
    Devices3_phys_base = 0x10400000,
    Devices4_phys_base = 0x12d00000,
    Devices5_phys_base = 0x02000000,
  };

  enum Virt_layout_exynos5 : Address {
    Uart2_map_base  = Devices2_map_base + 0x20000,
  };
};

INTERFACE [arm && exynos5_arndale]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_exynos5_arndale : Address {
    Uart_base         = Uart2_map_base,
    Sdram_phys_base   = 0x40000000,
    Gic_cpu_map_base  = Devices3_map_base + 0x82000,
    Gic_dist_map_base = Devices3_map_base + 0x81000,
    Timer_map_base    = Devices4_map_base + 0xd0000,
  };
};

