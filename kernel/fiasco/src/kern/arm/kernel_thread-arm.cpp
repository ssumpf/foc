IMPLEMENTATION [arm]:

#include "boot_info.h"
#include "config.h"

IMPLEMENT inline
void
Kernel_thread::free_initcall_section()
{
  //memset( &_initcall_start, 0, &_initcall_end - &_initcall_start );
}

IMPLEMENT FIASCO_INIT
void
Kernel_thread::bootstrap_arch()
{
  Proc::sti();

  boot_app_cpus();
}

//--------------------------------------------------------------------------
IMPLEMENTATION [!mp]:

static void inline
Kernel_thread::boot_app_cpus()
{}


//--------------------------------------------------------------------------
IMPLEMENTATION [mp]:

#include "io.h"
#include "pagetable.h"
#include "platform_control.h"
#include "outer_cache.h"

#include <cstdio>

static void
Kernel_thread::boot_app_cpus()
{
  extern char _tramp_mp_entry[];
  extern volatile Mword _tramp_mp_startup_cp15_c1;
  extern volatile Mword _tramp_mp_startup_pdbr;
  extern volatile Mword _tramp_mp_start_dcr;

  unsigned num_ap_cpus = Cpu::num_cpus();

  printf("Number of CPUs: %d\n", num_ap_cpus);

  _tramp_mp_startup_cp15_c1 = Config::Cache_enabled
                              ? Cpu::Cp15_c1_cache_enabled : Cpu::Cp15_c1_cache_disabled;
  _tramp_mp_startup_pdbr
    = Mem_space::kernel_space()->virt_to_phys((Address)Mem_space::kernel_space()->dir())
      | Page_table::Ttbr_bits;
  _tramp_mp_start_dcr       = 0x55555555;

  __asm__ __volatile__ ("" : : : "memory");
  Mem_unit::clean_dcache();

  Outer_cache::clean(Mem_space::kernel_space()->virt_to_phys((Address)&_tramp_mp_startup_cp15_c1));
  Outer_cache::clean(Mem_space::kernel_space()->virt_to_phys((Address)&_tramp_mp_startup_pdbr));
  Outer_cache::clean(Mem_space::kernel_space()->virt_to_phys((Address)&_tramp_mp_start_dcr));

  Platform_control::boot_ap_cpus(Mem_space::kernel_space()->virt_to_phys((Address)_tramp_mp_entry));
}

//--------------------------------------------------------------------------
IMPLEMENTATION [arm && generic_tickless_idle]:

#include "mem_unit.h"
#include "processor.h"

PROTECTED inline NEEDS["processor.h", "mem_unit.h"]
void
Kernel_thread::arch_tickless_idle(unsigned)
{
  Mem_unit::tlb_flush();
  Proc::halt();
}

PROTECTED inline NEEDS["processor.h"]
void
Kernel_thread::arch_idle(unsigned)
{ Proc::halt(); }

