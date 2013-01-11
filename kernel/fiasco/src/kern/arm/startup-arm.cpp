IMPLEMENTATION [arm]:

#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "fpu.h"
#include "ipi.h"
#include "kern_lib_page.h"
#include "kernel_task.h"
#include "kip_init.h"
#include "kmem_alloc.h"
#include "kmem_space.h"
#include "per_cpu_data.h"
#include "per_cpu_data_alloc.h"
#include "perf_cnt.h"
#include "pic.h"
#include "processor.h"
#include "static_init.h"
#include "timer.h"
#include "utcb_init.h"

#include <cstdlib>
#include <cstdio>

IMPLEMENT FIASCO_INIT FIASCO_NOINLINE
void
Startup::stage1()
{
  Proc::cli();
  Boot_info::init();
  Cpu::early_init();
  Config::init();
}

IMPLEMENT FIASCO_INIT FIASCO_NOINLINE
void
Startup::stage2()
{
  puts("Hello from Startup::stage2");

  // The first 4MB of phys memory are always mapped to Map_base
  Mem_layout::add_pmem(Mem_layout::Sdram_phys_base, Mem_layout::Map_base,
                       4 << 20);
  Kip_init::init();
  Kmem_alloc::init();

  // Initialize cpu-local data management and run constructors for CPU 0
  Per_cpu_data::init_ctors();
  Per_cpu_data_alloc::alloc(0);
  Per_cpu_data::run_ctors(0);

  Kmem_space::init();
  Kernel_task::init();
  Mem_space::kernel_space(Kernel_task::kernel_task());
  Pic::init();

  Cpu::init_mmu();
  Cpu::cpus.cpu(0).init(true);
  Fpu::init(0);
  Ipi::init(0);
  Timer::init(0);
  Kern_lib_page::init();
  Utcb_init::init();
}
