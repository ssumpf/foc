//--------------------------------------------------------------------------
IMPLEMENTATION [arm && tickless_idle && exynos]:

#include "cpu.h"
#include "pic.h"
#include "platform_control.h"
#include "processor.h"
#include "scheduler.h"

PROTECTED inline NEEDS["processor.h", "cpu.h", "platform_control.h", "scheduler.h"]
void
Kernel_thread::arch_tickless_idle(Cpu_number)
{
  Proc::halt();
}

PROTECTED inline NEEDS["processor.h"]
void
Kernel_thread::arch_idle(Cpu_number)
{
  Proc::halt();
}
