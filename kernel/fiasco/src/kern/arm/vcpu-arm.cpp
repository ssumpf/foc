INTERFACE [arm]:

#include "l4_types.h"

EXTENSION class Vcpu_state
{
public:
  Mword host_tpidruro;
  Mword user_tpidruro;
};
