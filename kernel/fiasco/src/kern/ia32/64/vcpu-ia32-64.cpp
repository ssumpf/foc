INTERFACE [amd64]:

#include "l4_types.h"

EXTENSION class Vcpu_state
{
public:
  Mword host_fs_base;
  Mword host_gs_base;
  Unsigned16 host_ds, host_es, host_fs, host_gs;

  Mword user_fs_base;
  Mword user_gs_base;
  Unsigned16 user_ds, user_es, user_fs, user_gs;

  Unsigned16 user_ds32;
  Unsigned16 user_cs64;
  Unsigned16 user_cs32;
};
