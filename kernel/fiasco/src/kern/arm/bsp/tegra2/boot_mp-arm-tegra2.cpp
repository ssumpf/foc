INTERFACE [arm && mp && tegra2]:

#include "mem_layout.h"

class Boot_mp
{
private:
  enum
  {
    Reset_vector_addr              = Mem_layout::Devices1_map_base + 0xf100,
    Clk_rst_ctrl_clk_cpu_cmplx     = Mem_layout::Devices1_map_base + 0x604c,
    Clk_rst_ctrl_rst_cpu_cmplx_clr = Mem_layout::Devices1_map_base + 0x6344,
    Unhalt_addr                    = Mem_layout::Devices1_map_base + 0x7014,
  };

  Mword _orig_reset_vector;
};

IMPLEMENTATION [arm && mp && tegra2]:

#include "io.h"

PUBLIC
void
Boot_mp::start_ap_cpus(Address phys_reset_vector)
{
  // remember original reset vector
  _orig_reset_vector = Io::read<Mword>(Reset_vector_addr);

  // set (temporary) new reset vector
  Io::write<Mword>(phys_reset_vector, Reset_vector_addr);

  // clocks on other cpu
  Mword r = Io::read<Mword>(Clk_rst_ctrl_clk_cpu_cmplx);
  Io::write<Mword>(r & ~(1 << 9), Clk_rst_ctrl_clk_cpu_cmplx);
  Io::write<Mword>((1 << 13) | (1 << 9) | (1 << 5) | (1 << 1),
		   Clk_rst_ctrl_rst_cpu_cmplx_clr);

  // kick cpu1
  Io::write<Mword>(0, Unhalt_addr);
}

PUBLIC
void
Boot_mp::cleanup()
{
  Io::write<Mword>(_orig_reset_vector, Reset_vector_addr);
}
