IMPLEMENTATION [arm && omap3_35x]: //--------------------------------------

#include "io.h"
#include "kmem.h"

void __attribute__ ((noreturn))
platform_reset(void)
{
  enum
    {
      PRM_RSTCTRL = Kmem::Prm_global_reg_map_base + 0x50,
    };

  Io::write<Mword>(2, PRM_RSTCTRL);

  for (;;)
    ;
}

IMPLEMENTATION [arm && omap3_am33xx]: //-----------------------------------

#include "io.h"
#include "kmem.h"

void __attribute__ ((noreturn))
platform_reset(void)
{
  enum { PRM_RSTCTRL = Kmem::Devices1_map_base + 0xF00, };

  Io::write<Mword>(1, PRM_RSTCTRL);

  for (;;)
    ;
}

IMPLEMENTATION [arm && omap4]: //------------------------------------------

#include "io.h"
#include "kmem.h"

void __attribute__ ((noreturn))
platform_reset(void)
{
  enum
    {
      DEVICE_PRM  = Kmem::Prm_map_base + 0x1b00,
      PRM_RSTCTRL = DEVICE_PRM + 0,
    };

  Io::write<Mword>(1, PRM_RSTCTRL);

  for (;;)
    ;
}
