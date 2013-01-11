IMPLEMENTATION [arm && sa1100]:

#include <sa1100.h>
#include "kmem.h"
#include "io.h"

typedef Sa1100_generic<Kmem::Timer_map_base> Sa1100;

void __attribute__ ((noreturn))
platform_reset(void)
{
  Io::write( (Mword)Sa1100::RSRR_SWR, (Address)Sa1100::RSRR );
  for (;;)
    ;
}
