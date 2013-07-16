IMPLEMENTATION [arm && exynos && outer_cache_l2cxx0]:

#include "platform.h"
#include "smc.h"

IMPLEMENT
Mword
Outer_cache::platform_init(Mword auxc)
{
  // power control
  enum {
    Standby_mode_enable       = 1 << 0,
    Dynamic_clk_gating_enable = 1 << 1,
  };

  unsigned tag_lat = 0x110;
  unsigned data_lat = Platform::is_4210() ? 0x110 : 0x120;
  unsigned prefctrl = 0x30000007;
  if (Platform::is_4412() && Platform::subrev() > 0x10)
    prefctrl = 0x71000007;

  Mword auxc_mask = 0xc200fffe;
  Mword auxc_bits =   (1 <<  0)  // Full Line of Zero Enable
                    | (1 << 16)  // 16 way
                    | (3 << 17)  // way size == 64KB
                    | (1 << 22)  // shared ignored
                    | (3 << 26)  // tz
                    | (1 << 28)  // data prefetch enable
                    | (1 << 29)  // insn prefetch enable
                    | (1 << 30)  // early BRESP enable
                    ;

  if (!Platform::running_ns())
    {
      l2cxx0->write<Mword>(tag_lat, L2cxx0::TAG_RAM_CONTROL);
      l2cxx0->write<Mword>(data_lat, L2cxx0::DATA_RAM_CONTROL);

      l2cxx0->write<Mword>(prefctrl, 0xf60);

      l2cxx0->write<Mword>(Standby_mode_enable | Dynamic_clk_gating_enable,
                           0xf80);
    }
  else
    Exynos_smc::l2cache_setup(tag_lat, data_lat, prefctrl,
                              Standby_mode_enable | Dynamic_clk_gating_enable,
                              auxc_bits, auxc_mask);

  return (auxc & auxc_mask) | auxc_bits;
}
