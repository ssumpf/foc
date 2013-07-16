INTERFACE:

#include "types.h"

class Platform_control
{
public:
  static void init(Cpu_number);
  static bool cpu_offline_available();
  static int resume_cpu(Cpu_number cpu);
  static int suspend_cpu(Cpu_number cpu);
  static int system_suspend();
};

// ------------------------------------------------------------------------
IMPLEMENTATION:

#include "l4_types.h"

IMPLEMENT_DEFAULT inline
void
Platform_control::init(Cpu_number)
{}

IMPLEMENT_DEFAULT inline NEEDS["l4_types.h"]
int
Platform_control::system_suspend()
{ return -L4_err::EBusy; }

IMPLEMENT_DEFAULT inline
bool
Platform_control::cpu_offline_available()
{ return false; }

IMPLEMENT_DEFAULT inline NEEDS["l4_types.h"]
int
Platform_control::suspend_cpu(Cpu_number)
{ return -L4_err::ENodev; }

IMPLEMENT_DEFAULT inline NEEDS["l4_types.h"]
int
Platform_control::resume_cpu(Cpu_number)
{ return -L4_err::ENodev; }
