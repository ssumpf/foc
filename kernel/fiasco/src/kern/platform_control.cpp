INTERFACE:

class Platform_control
{
public:
  static void init(unsigned);
  static bool cpu_offline_available();
  static int resume_cpu(unsigned cpu);
  static int suspend_cpu(unsigned cpu);
  static int system_suspend();
};

// ------------------------------------------------------------------------
IMPLEMENTATION [!cpu_suspend]:

#include "l4_types.h"

IMPLEMENT inline
void
Platform_control::init(unsigned)
{}

IMPLEMENT inline
bool
Platform_control::cpu_offline_available()
{ return false; }

IMPLEMENT inline NEEDS["l4_types.h"]
int
Platform_control::suspend_cpu(unsigned)
{ return -L4_err::ENodev; }

IMPLEMENT inline NEEDS["l4_types.h"]
int
Platform_control::resume_cpu(unsigned)
{ return -L4_err::ENodev; }

IMPLEMENT inline NEEDS["l4_types.h"]
int
Platform_control::system_suspend()
{ return -L4_err::EBusy; }
