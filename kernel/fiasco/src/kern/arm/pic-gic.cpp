//-------------------------------------------------------------------
IMPLEMENTATION [arm && pic_gic]:

#include "gic.h"
#include "initcalls.h"

EXTENSION class Pic
{
public:
  static Static_object<Gic> gic;
};

Static_object<Gic> Pic::gic;

extern "C"
void irq_handler()
{ Pic::gic->hit(0); }

