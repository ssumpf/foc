INTERFACE [arm && tegra2]:

EXTENSION class Mem_layout
{
public:
  enum Virt_layout_tegra2 {

    Devices0_map_base    = Registers_map_start,
    Devices1_map_base    = Registers_map_start + 0x00100000,
    Devices2_map_base    = Registers_map_start + 0x00200000,

    Mp_scu_map_base      = Devices2_map_base + 0x00040000,
    L2cxx0_map_base      = Devices2_map_base + 0x00043000,

    Gic_cpu_map_base     = Devices2_map_base + 0x00040100,
    Gic_dist_map_base    = Devices2_map_base + 0x00041000,
    Gic2_cpu_map_base    = Devices2_map_base + 0x00020000,
    Gic2_dist_map_base   = Devices2_map_base + 0x00021000,

    Uart_base            = Devices0_map_base + 0x00006300,
    Clock_reset_map_base = Devices1_map_base + 0x00006000,
  };

  enum Phys_layout_tegra2 {
    Devices0_phys_base   = 0x70000000,
    Devices1_phys_base   = 0x60000000,
    Devices2_phys_base   = 0x50000000,
    Sdram_phys_base      = 0x0,
  };
};
