INTERFACE [sparc]:

#include "types.h"
#include "sparc_types.h"

EXTENSION class Boot_info 
{
  public:
    /**
     * Return memory-mapped base address of uart/pic
     */
    static Address uart_base();
    static Address pic_base();
};


//------------------------------------------------------------------------------
IMPLEMENTATION [sparc]:

#include "boot_info.h"
#include <string.h>

IMPLEMENT static 
void Boot_info::init()
{
}


