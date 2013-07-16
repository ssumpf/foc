IMPLEMENTATION [arm]:

#include "globals.h"
#include "kernel_task.h"
#include "kmem_alloc.h"
#include "kmem_space.h"
#include "space.h"
#include "mem_layout.h"
#include "mem_unit.h"
#include "static_init.h"
#include "watchdog.h"
#include "cxx/cxx_int"

STATIC_INITIALIZE_P(Jdb, JDB_INIT_PRIO);

DEFINE_PER_CPU static Per_cpu<Proc::Status> jdb_irq_state;

// disable interrupts before entering the kernel debugger
IMPLEMENT
void
Jdb::save_disable_irqs(Cpu_number cpu)
{
  jdb_irq_state.cpu(cpu) = Proc::cli_save();
  if (cpu == Cpu_number::boot_cpu())
    Watchdog::disable();
}

// restore interrupts after leaving the kernel debugger
IMPLEMENT
void
Jdb::restore_irqs(Cpu_number cpu)
{
  if (cpu == Cpu_number::boot_cpu())
    Watchdog::enable();
  Proc::sti_restore(jdb_irq_state.cpu(cpu));
}

IMPLEMENT inline
void
Jdb::enter_trap_handler(Cpu_number)
{}

IMPLEMENT inline
void
Jdb::leave_trap_handler(Cpu_number)
{}

PROTECTED static inline
void
Jdb::monitor_address(Cpu_number, void *)
{}

IMPLEMENT inline
bool
Jdb::handle_conditional_breakpoint(Cpu_number)
{ return false; }

IMPLEMENT
void
Jdb::handle_nested_trap(Jdb_entry_frame *e)
{
  printf("Trap in JDB: IP:%08lx PSR=%08lx ERR=%08lx\n",
         e->ip(), e->psr, e->error_code);
}

IMPLEMENT
bool
Jdb::handle_debug_traps(Cpu_number cpu)
{
  Jdb_entry_frame *ef = entry_frame.cpu(cpu);

  if (ef->error_code == 0x00e00000)
    snprintf(error_buffer.cpu(cpu), sizeof(error_buffer.cpu(Cpu_number::first())), "%s",
             (char const *)ef->r[0]);
  else if (ef->debug_ipi())
    snprintf(error_buffer.cpu(cpu), sizeof(error_buffer.cpu(Cpu_number::first())),
             "IPI ENTRY");

  return true;
}

IMPLEMENT inline
bool
Jdb::handle_user_request(Cpu_number cpu)
{
  Jdb_entry_frame *ef = Jdb::entry_frame.cpu(cpu);
  const char *str = (char const *)ef->r[0];
  Space * task = get_task(cpu);
  char tmp;

  if (ef->debug_ipi())
    return cpu != Cpu_number::boot_cpu();

  if (ef->error_code == 0x00e00001)
    return execute_command_ni(task, str);

  if (!peek(str, task, tmp) || tmp != '*')
    return false;
  if (!peek(str+1, task, tmp) || tmp != '#')
    return false;

  return execute_command_ni(task, str+2);
}

IMPLEMENT inline
bool
Jdb::test_checksums()
{ return true; }

static
bool
Jdb::handle_special_cmds(int)
{ return 1; }

PUBLIC static
FIASCO_INIT FIASCO_NOINLINE void
Jdb::init()
{
  static Jdb_handler enter(at_jdb_enter);
  static Jdb_handler leave(at_jdb_leave);

  Jdb::jdb_enter.add(&enter);
  Jdb::jdb_leave.add(&leave);

  Thread::nested_trap_handler = (Trap_state::Handler)enter_jdb;

  Kconsole::console()->register_console(push_cons());
}


PRIVATE static
void *
Jdb::access_mem_task(Address virt, Space * task)
{
  // align
  virt &= ~0x03;

  Address phys;

  if (!task)
    {
      if (Mem_layout::in_kernel(virt))
	{
	  auto p = Kmem_space::kdir()->walk(Virt_addr(virt));
	  if (!p.is_valid())
	    return 0;

	  phys = p.page_addr() | cxx::get_lsb(virt, p.page_order());
	}
      else
	phys = virt;
    }
  else
    {
      phys = Address(task->virt_to_phys(virt));


      if (phys == (Address)-1)
	phys = task->virt_to_phys_s0((void *)virt);

      if (phys == (Address)-1)
	return 0;
    }

  unsigned long addr = Mem_layout::phys_to_pmem(phys);
  if (addr == (Address)-1)
    {
      Mem_unit::flush_vdcache();
      auto pte = static_cast<Mem_space*>(Kernel_task::kernel_task())
        ->_dir->walk(Virt_addr(Mem_layout::Jdb_tmp_map_area), Pdir::Super_level);

      if (!pte.is_valid() || pte.page_addr() != cxx::mask_lsb(phys, pte.page_order()))
        {
          pte.create_page(Phys_mem_addr(cxx::mask_lsb(phys, pte.page_order())),
                          Page::Attr(Page::Rights::RW()));
          pte.write_back_if(true, Mem_unit::Asid_kernel);
        }

      Mem_unit::tlb_flush();

      addr = Mem_layout::Jdb_tmp_map_area + (phys & (Config::SUPERPAGE_SIZE - 1));
    }

  return (Mword*)addr;
}

PUBLIC static
Space *
Jdb::translate_task(Address addr, Space * task)
{
  return (Kmem::is_kmem_page_fault(addr, 0)) ? 0 : task;
}

PUBLIC static
int
Jdb::peek_task(Address virt, Space * task, void *value, int width)
{
  void const *mem = access_mem_task(virt, task);
  if (!mem)
    return -1;

  switch (width)
    {
    case 1:
        {
          Mword dealign = (virt & 0x3) * 8;
          *(Mword*)value = (*(Mword*)mem & (0xff << dealign)) >> dealign;
        }
	break;
    case 2:
        {
          Mword dealign = ((virt & 0x2) >> 1) * 16;
          *(Mword*)value = (*(Mword*)mem & (0xffff << dealign)) >> dealign;
        }
	break;
    case 4:
      memcpy(value, mem, width);
    }

  return 0;
}

PUBLIC static
int
Jdb::is_adapter_memory(Address, Space *)
{
  return 0;
}

PUBLIC static
int
Jdb::poke_task(Address virt, Space * task, void const *val, int width)
{
  void *mem = access_mem_task(virt, task);
  if (!mem)
    return -1;

  memcpy(mem, val, width);
  return 0;
}


PRIVATE static
void
Jdb::at_jdb_enter()
{
  Mem_unit::clean_vdcache();
}

PRIVATE static
void
Jdb::at_jdb_leave()
{
  Mem_unit::flush_vcache();
}

PUBLIC static inline
void
Jdb::enter_getchar()
{}

PUBLIC static inline
void
Jdb::leave_getchar()
{}

PUBLIC static
void
Jdb::write_tsc_s(Signed64 tsc, char *buf, int maxlen, bool sign)
{
  if (sign)
    {
      *buf++ = (tsc < 0) ? '-' : (tsc == 0) ? ' ' : '+';
      maxlen--;
    }
  snprintf(buf, maxlen, "%lld c", tsc);
}

PUBLIC static
void
Jdb::write_tsc(Signed64 tsc, char *buf, int maxlen, bool sign)
{
  write_tsc_s(tsc, buf, maxlen, sign);
}

//----------------------------------------------------------------------------
IMPLEMENTATION [arm && !mp]:

PROTECTED static inline
template< typename T >
void
Jdb::set_monitored_address(T *dest, T val)
{ *dest = val; }

//----------------------------------------------------------------------------
IMPLEMENTATION [arm && mp]:

#include <cstdio>

static
void
Jdb::send_nmi(Cpu_number cpu)
{
  printf("NMI to %d, what's that?\n",
         cxx::int_value<Cpu_number>(cpu));
}

PROTECTED static inline
template< typename T >
void
Jdb::set_monitored_address(T *dest, T val)
{
  *dest = val;
  Mem::dsb();
  asm volatile("sev");
}

PROTECTED static inline
template< typename T >
T Jdb::monitor_address(Cpu_number, T volatile *addr)
{
  asm volatile("wfe");
  return *addr;
}
