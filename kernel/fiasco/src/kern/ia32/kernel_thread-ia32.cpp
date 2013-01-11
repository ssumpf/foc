
IMPLEMENTATION[ia32,amd64]:

#include "apic.h"
#include "config.h"
#include "cpu.h"
#include "io_apic.h"
#include "irq_mgr.h"
#include "koptions.h"
#include "mem_layout.h"
#include "pic.h"
#include "trap_state.h"
#include "watchdog.h"

IMPLEMENT inline NEEDS["mem_layout.h"]
void
Kernel_thread::free_initcall_section()
{
  // just fill up with invalid opcodes
  for (unsigned short *i = (unsigned short *) &Mem_layout::initcall_start;   
                       i < (unsigned short *) &Mem_layout::initcall_end; i++)
    *i = 0x0b0f;	// UD2 opcode
}


IMPLEMENT FIASCO_INIT
void
Kernel_thread::bootstrap_arch()
{
  // 
  // install our slow trap handler
  //
  nested_trap_handler      = Trap_state::base_handler;
  Trap_state::base_handler = thread_handle_trap;

  // initialize the profiling timer
  bool user_irq0 = Koptions::o()->opt(Koptions::F_irq0);

  if ((int)Config::Scheduler_mode == Config::SCHED_PIT && user_irq0)
    panic("option -irq0 not possible since irq 0 is used for scheduling");

  boot_app_cpus();
}

//--------------------------------------------------------------------------
IMPLEMENTATION [!mp]:

static void inline
Kernel_thread::boot_app_cpus()
{}


//--------------------------------------------------------------------------
IMPLEMENTATION [mp && amd64]:

PRIVATE inline NOEXPORT
static Address
Kernel_thread::get_startup_pbdr()
{
  // for amd64 we need to make sure that our boot-up page directory is below
  // 4GB in physical memory
  static char _boot_pdir_page[Config::PAGE_SIZE] __attribute__((aligned(4096)));
  memcpy(_boot_pdir_page, Kmem::dir(), sizeof(_boot_pdir_page));

  return Kmem::virt_to_phys(_boot_pdir_page);
}

//--------------------------------------------------------------------------
IMPLEMENTATION [mp && ia32]:

PRIVATE inline NOEXPORT
static Address
Kernel_thread::get_startup_pbdr()
{
  return Mem_layout::pmem_to_phys(Kmem::dir());
}

//--------------------------------------------------------------------------
IMPLEMENTATION [mp]:

static void
Kernel_thread::boot_app_cpus()
{
  // sending (INIT-)IPIs on non-MP systems might not work
  if (   Cpu::boot_cpu()->vendor() == Cpu::Vendor_amd
      && Cpu::amd_cpuid_mnc() < 2)
    return;

  // where to start the APs for detection of the APIC-IDs
  extern char _tramp_mp_entry[];

  // feature enabling flags (esp. cache enabled flag and paging enabled flag)
  extern volatile Mword _tramp_mp_startup_cr0;

  // feature enabling flags (esp. needed for big pages)
  extern volatile Mword _tramp_mp_startup_cr4;

  // physical address of the page table directory to use
  extern volatile Address _tramp_mp_startup_pdbr;

  // pseudo descriptor for the gdt to load
  extern Pseudo_descriptor _tramp_mp_startup_gdt_pdesc;

  Address tramp_page;

  _tramp_mp_startup_pdbr = get_startup_pbdr();
  _tramp_mp_startup_cr4 = Cpu::get_cr4();
  _tramp_mp_startup_cr0 = Cpu::get_cr0();
  _tramp_mp_startup_gdt_pdesc
    = Pseudo_descriptor((Address)Cpu::boot_cpu()->get_gdt(), Gdt::gdt_max -1);

  __asm__ __volatile__ ("" : : : "memory");

  // Say what we do
  printf("MP: detecting APs...\n");

  // broadcast an AP startup via the APIC (let run the self-registration code)
  tramp_page = (Address)&(_tramp_mp_entry[0]);

  // Send IPI-Sequency to startup the APs
  Apic::mp_startup(Cpu::boot_cpu(), Apic::APIC_IPI_OTHERS, tramp_page);
}
