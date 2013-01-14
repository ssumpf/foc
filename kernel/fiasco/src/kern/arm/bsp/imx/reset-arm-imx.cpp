IMPLEMENTATION [arm && imx21]:

#include "io.h"
#include "kmem.h"

void __attribute__ ((noreturn))
platform_reset(void)
{
  enum {
    WCR  = Kmem::Watchdog_map_base + 0,
    WCR_SRS = 1 << 4, // Software Reset Signal

    PLL_PCCR1        = Kmem::Pll_map_base + 0x24,
    PLL_PCCR1_WDT_EN = 1 << 24,
  };

  // WDT CLock Enable
  Io::write<Unsigned32>(Io::read<Unsigned32>(PLL_PCCR1) | PLL_PCCR1_WDT_EN, PLL_PCCR1);

  // Assert Software reset signal by making the bit zero
  Io::write<Unsigned16>(Io::read<Unsigned16>(WCR) & ~WCR_SRS, WCR);

  for (;;)
    ;
}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && (imx35 || imx51 || imx53)]:

void platform_imx_cpus_off()
{}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && imx6]:

void platform_imx_cpus_off()
{
  // switch off core1-3
  Io::clear<Mword>(7 << 22, Mem_layout::Src_map_base + 0);
}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && (imx35 || imx51 || imx53 || imx6)]:

#include "io.h"
#include "kmem.h"

void __attribute__ ((noreturn))
platform_reset(void)
{
  enum {
    WCR  = Kmem::Watchdog_map_base + 0,
    WCR_SRS = 1 << 4, // Software Reset Signal
  };

  platform_imx_cpus_off();

  // Assert Software reset signal by making the bit zero
  Io::write<Unsigned16>(Io::read<Unsigned16>(WCR) & ~WCR_SRS, WCR);

  for (;;)
    ;
}
