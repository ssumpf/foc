IMPLEMENTATION [arm && exynos5]:

#include "panic.h"
#include "warn.h"
#include "initcalls.h"

IMPLEMENT FIASCO_INIT
void
Pic::init()
{
  NOT_IMPL_PANIC;
}

IMPLEMENT inline
Pic::Status Pic::disable_all_save()
{ return 0; }

IMPLEMENT inline
void Pic::restore_all(Status)
{}

extern "C"
void irq_handler()
{
	NOT_IMPL_PANIC;
}
