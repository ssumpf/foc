IMPLEMENTATION [arm && arm_em_tz]:

#include "ram_quota.h"
#include "vm.h"

IMPLEMENT
Vm *
Vm_factory::create(Ram_quota *quota, int *err)
{
  if (void *t = Vm::allocator()->q_alloc(quota))
    {
      Vm *a = new (t) Vm(quota);
      if (a->initialize())
        return a;

      delete a;
    }

  *err = -L4_err::ENomem;
  return 0;
}
