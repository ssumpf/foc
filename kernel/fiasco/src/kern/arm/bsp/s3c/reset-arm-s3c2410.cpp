IMPLEMENTATION [arm && s3c2410]:

#include "io.h"
#include "kmem.h"

void __attribute__ ((noreturn))
platform_reset(void)
{
  enum {
    WTCON = Kmem::Watchdog_map_base + 0x0,
    WTDAT = Kmem::Watchdog_map_base + 0x4,
    WTCNT = Kmem::Watchdog_map_base + 0x8,

    WTCON_RST_EN    = 1 << 0,
    WTCON_EN        = 1 << 5,
    WTCON_PRESCALER = (0x10 << 8),
  };

  Io::write(0, WTCON); // disable
  Io::write(200, WTDAT); // set initial values
  Io::write(200, WTCNT);

  Io::write(WTCON_RST_EN | WTCON_EN | WTCON_PRESCALER, WTCON);

  // we should reboot now
  while (1)
    ;
}
