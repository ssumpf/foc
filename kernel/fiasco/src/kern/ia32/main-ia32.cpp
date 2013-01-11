/*
 * Fiasco-IA32/AMD64
 * Architecture specific main startup/shutdown code
 */

IMPLEMENTATION[ia32,amd64]:

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "config.h"
#include "io.h"
#include "idt.h"
#include "kdb_ke.h"
#include "kernel_console.h"
#include "koptions.h"
#include "pic.h"
#include "processor.h"
#include "reset.h"
#include "timer.h"
#include "timer_tick.h"
#include "terminate.h"

static int exit_question_active;


extern "C" void __attribute__ ((noreturn))
_exit(int)
{
  if (exit_question_active)
    platform_reset();

  while (1)
    {
      Proc::halt();
      Proc::pause();
    }
}


static
void
exit_question()
{
  Proc::cli();
  exit_question_active = 1;

  Pic::Status irqs = Pic::disable_all_save();
  if (Config::getchar_does_hlt_works_ok)
    {
      Timer_tick::set_vectors_stop();
      Timer_tick::enable(0); // hmexit alway on CPU 0
      Proc::sti();
    }

  // make sure that we don't acknowledg the exit question automatically
  Kconsole::console()->change_state(Console::PUSH, 0, ~Console::INENABLED, 0);
  puts("\nReturn reboots, \"k\" enters L4 kernel debugger...");

  char c = Kconsole::console()->getchar();

  if (c == 'k' || c == 'K') 
    {
      Pic::restore_all(irqs);
      kdb_ke("_exit");
    }
  else
    {
      // It may be better to not call all the destruction stuff because of
      // unresolved static destructor dependency problems. So just do the
      // reset at this point.
      puts("\033[1mRebooting.\033[m");
    }
}

void
main_arch()
{
  // console initialization
  set_exit_question(&exit_question);

  //Pic::disable_all_save();
}


//------------------------------------------------------------------------
IMPLEMENTATION[(ia32,amd64) && mp]:

#include <cstdio>
#include "apic.h"
#include "app_cpu_thread.h"
#include "config.h"
#include "cpu.h"
#include "div32.h"
#include "fpu.h"
#include "globals.h"
#include "ipi.h"
#include "kernel_task.h"
#include "processor.h"
#include "per_cpu_data_alloc.h"
#include "perf_cnt.h"
#include "spin_lock.h"
#include "utcb_init.h"

int FIASCO_FASTCALL boot_ap_cpu(unsigned _cpu) __asm__("BOOT_AP_CPU");

int FIASCO_FASTCALL boot_ap_cpu(unsigned _cpu)
{
  if (!Per_cpu_data_alloc::alloc(_cpu))
    {
      extern Spin_lock<Mword> _tramp_mp_spinlock;
      printf("CPU allocation failed for CPU%u, disabling CPU.\n", _cpu);
      _tramp_mp_spinlock.clear();
      while (1)
	Proc::halt();
    }
  Per_cpu_data::run_ctors(_cpu);
  Cpu &cpu = Cpu::cpus.cpu(_cpu);

  Kmem::init_cpu(cpu);
  Idt::load();

  Apic::init_ap();
  Ipi::init(_cpu);
  Timer::init(_cpu);
  Apic::check_still_getting_interrupts();


  // caution: no stack variables in this function because we're going
  // to change the stack pointer!
  cpu.print();
  cpu.show_cache_tlb_info("");

  if (Koptions::o()->opt(Koptions::F_loadcnt))
    Perf_cnt::init_ap();

  puts("");

  // create kernel thread
  App_cpu_thread *kernel = new (Ram_quota::root) App_cpu_thread();
  set_cpu_of(kernel, _cpu);
  check(kernel->bind(Kernel_task::kernel_task(), User<Utcb>::Ptr(0)));

  main_switch_ap_cpu_stack(kernel);
  return 0;
}
