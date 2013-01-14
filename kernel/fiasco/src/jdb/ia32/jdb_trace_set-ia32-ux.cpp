IMPLEMENTATION:
#include "syscalls.h"
#include "idt.h"
#include "jdb.h"

extern "C" void entry_sys_ipc_log (void);
extern "C" void entry_sys_ipc_c (void);
extern "C" void entry_sys_ipc (void);
extern "C" void entry_sys_fast_ipc_log (void);
extern "C" void entry_sys_fast_ipc_c (void);
extern "C" void entry_sys_fast_ipc (void);

extern "C" void sys_ipc_wrapper (void);
extern "C" void ipc_short_cut_wrapper (void);
extern "C" void sys_ipc_log_wrapper (void);
extern "C" void sys_ipc_trace_wrapper (void);

typedef void (Fast_entry_func)(void);

PUBLIC static
void
Jdb_set_trace::ia32_set_fast_entry(unsigned cpu, void *entry)
{
  Cpu::cpus.cpu(cpu).set_fast_entry((Fast_entry_func*)entry);
}

struct Set_fast_entry
{
  void *entry;
  Set_fast_entry(Fast_entry_func *entry) : entry((void*)entry) {}
  void operator () (unsigned cpu) const
  { Jdb::remote_work(cpu, Jdb_set_trace::ia32_set_fast_entry, entry, true); }
};

static
void
Jdb_set_trace::set_ipc_vector()
{
  void (*int30_entry)(void);
  void (*fast_entry)(void);

  if (Jdb_ipc_trace::_trace || Jdb_ipc_trace::_slow_ipc || 
      Jdb_ipc_trace::_log   || Jdb_nextper_trace::_log)
    {
      int30_entry = entry_sys_ipc_log;
      fast_entry  = entry_sys_fast_ipc_log;
    }
  else
    {
      int30_entry = entry_sys_ipc_c;
      fast_entry  = entry_sys_fast_ipc_c;
    }

  Idt::set_entry(0x30, (Address) int30_entry, true);
  Jdb::foreach_cpu(Set_fast_entry(fast_entry));

  if (Jdb_ipc_trace::_trace)
    syscall_table[0] = sys_ipc_trace_wrapper;
  else if ((Jdb_ipc_trace::_log && !Jdb_ipc_trace::_slow_ipc) ||
	   Jdb_nextper_trace::_log)            
    syscall_table[0] = sys_ipc_log_wrapper;
  else
    syscall_table[0] = sys_ipc_wrapper;
}

PUBLIC static FIASCO_NOINLINE
void
Jdb_set_trace::set_cpath()
{
  Jdb_ipc_trace::_cpath = 0;
  BEGIN_LOG_EVENT("Context switches", "csw", Tb_entry_empty)
  Jdb_ipc_trace::_cpath = 1;
  END_LOG_EVENT;
  BEGIN_LOG_EVENT("Shortcut", "sc", Tb_entry_empty)
  Jdb_ipc_trace::_cpath = 1;
  END_LOG_EVENT;
  set_ipc_vector();
}

void
jdb_trace_set_cpath(void)
{
  Jdb_set_trace::set_cpath();
}

IMPLEMENT void
Jdb_set_trace::next_preiod_tracing(bool enable)
{
  if (enable)
    Jdb_nextper_trace::_log = 1;
  else
    Jdb_nextper_trace::_log = 0;

  set_ipc_vector();
}

IMPLEMENT void
Jdb_set_trace::page_fault_tracing(bool /*enable*/)
{
}

IMPLEMENT void
Jdb_set_trace::ipc_tracing(Mode mode)
{
  switch (mode)
    {
    case Off:
      Jdb_ipc_trace::_trace = 0;
      Jdb_ipc_trace::_log = 0;
      Jdb_ipc_trace::_cshortcut = 0;
      Jdb_ipc_trace::_slow_ipc = 0;
      break;
    case Log:
      Jdb_ipc_trace::_trace = 0;
      Jdb_ipc_trace::_log = 1;
      Jdb_ipc_trace::_log_to_buf = 0;
      Jdb_ipc_trace::_cshortcut = 0;
      Jdb_ipc_trace::_slow_ipc = 0;
      break;
    case Log_to_buf:
      Jdb_ipc_trace::_trace = 0;
      Jdb_ipc_trace::_log = 1;
      Jdb_ipc_trace::_log_to_buf = 1;
      Jdb_ipc_trace::_cshortcut = 0;
      Jdb_ipc_trace::_slow_ipc = 0;
      break;
    case Trace:
      Jdb_ipc_trace::_trace = 1;
      Jdb_ipc_trace::_cshortcut = 0;
      Jdb_ipc_trace::_log = 0;
      Jdb_ipc_trace::_slow_ipc = 0;
      break;
    case Use_c_short_cut:
      Jdb_ipc_trace::_cshortcut = 1;
      break;
    case Use_slow_path:
      Jdb_ipc_trace::_slow_ipc = 1;
      break;
    }
  set_ipc_vector();
}


static
void
Jdb_set_trace::set_unmap_vector()
{
}

IMPLEMENT void
Jdb_set_trace::unmap_tracing(bool /*enable*/)
{
}
