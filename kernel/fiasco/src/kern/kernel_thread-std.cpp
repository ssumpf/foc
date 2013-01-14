IMPLEMENTATION:

#include "assert_opt.h"
#include "config.h"
#include "factory.h"
#include "initcalls.h"
#include "ipc_gate.h"
#include "irq.h"
#include "koptions.h"
#include "map_util.h"
#include "mem_layout.h"
#include "sigma0_task.h"
#include "task.h"
#include "thread_object.h"
#include "types.h"
#include "ram_quota.h"

enum Default_base_caps
{
  C_task      = 1,
  C_factory   = 2,
  C_thread    = 3,
  C_pager     = 4,
  C_log       = 5,
  C_icu       = 6,
  C_scheduler = 7

};


IMPLEMENT
void
Kernel_thread::init_workload()
{
  auto g = lock_guard(cpu_lock);

  if (Config::Jdb &&
      !Koptions::o()->opt(Koptions::F_nojdb) &&
      Koptions::o()->opt(Koptions::F_jdb_cmd))
    {
      // extract the control sequence from the command line
      char ctrl[128];
      char const *s = Koptions::o()->jdb_cmd;
      char *d;

      for (d=ctrl; d < ctrl+sizeof(ctrl)-1 && *s && *s != ' '; *d++ = *s++)
	;
      *d = '\0';

      kdb_ke_sequence(ctrl);
    }

  // kernel debugger rendezvous
  if (Koptions::o()->opt(Koptions::F_wait))
    kdb_ke("Wait");

  //
  // create sigma0
  //

  Task *sigma0 = Task::create<Sigma0_task>(Ram_quota::root,
      L4_fpage::mem(Mem_layout::Utcb_addr, Config::PAGE_SHIFT));

  assert_opt (sigma0);
  // prevent deletion of this thing
  sigma0->inc_ref();

  init_mapdb_mem(sigma0);
  init_mapdb_io(sigma0);

  check (map(sigma0,          sigma0, sigma0, C_task, 0));
  check (map(Factory::root(), sigma0, sigma0, C_factory, 0));

  for (unsigned c = Initial_kobjects::First_cap; c < Initial_kobjects::End_cap; ++c)
    {
      Kobject_iface *o = initial_kobjects.obj(c);
      if (o)
	check(map(o, sigma0, sigma0, c, 0));
    }

  Thread_object *sigma0_thread = new (Ram_quota::root) Thread_object();

  assert_kdb(sigma0_thread);

  // prevent deletion of this thing
  sigma0_thread->inc_ref();
  check (map(sigma0_thread, sigma0, sigma0, C_thread, 0));

  Address sp = init_workload_s0_stack();
  check (sigma0_thread->control(Thread_ptr(false), Thread_ptr(false)) == 0);
  check (sigma0_thread->bind(sigma0, User<Utcb>::Ptr((Utcb*)Mem_layout::Utcb_addr)));
  check (sigma0_thread->ex_regs(Kip::k()->sigma0_ip, sp));

  //
  // create the boot task
  //

  Task *boot_task = Task::create<Task>(Ram_quota::root,
      L4_fpage::mem(Mem_layout::Utcb_addr, Config::PAGE_SHIFT+2));

  assert_opt (boot_task);

  // prevent deletion of this thing
  boot_task->inc_ref();

  Thread_object *boot_thread = new (Ram_quota::root) Thread_object();

  assert_kdb (boot_thread);

  // prevent deletion of this thing
  boot_thread->inc_ref();

  check (map(boot_task,   boot_task, boot_task, C_task, 0));
  check (map(boot_thread, boot_task, boot_task, C_thread, 0));

  check (boot_thread->control(Thread_ptr(C_pager), Thread_ptr(~0UL)) == 0);
  check (boot_thread->bind(boot_task, User<Utcb>::Ptr((Utcb*)Mem_layout::Utcb_addr)));
  check (boot_thread->ex_regs(Kip::k()->root_ip, Kip::k()->root_sp));

  Ipc_gate *s0_b_gate = Ipc_gate::create(Ram_quota::root, sigma0_thread, 4 << 4);

  check (s0_b_gate);
  check (map(s0_b_gate, boot_task, boot_task, C_pager, 0));

  set_cpu_of(sigma0_thread, 0);
  set_cpu_of(boot_thread, 0);

  sigma0_thread->activate();
  check (obj_map(sigma0, C_factory,   1, boot_task, C_factory, 0).error() == 0);
  for (unsigned c = Initial_kobjects::First_cap; c < Initial_kobjects::End_cap; ++c)
    {
      Kobject_iface *o = initial_kobjects.obj(c);
      if (o)
	check(obj_map(sigma0, c, 1, boot_task, c, 0).error() == 0);
    }

  boot_thread->activate();
}

IMPLEMENTATION [ia32,amd64]:

PRIVATE inline
Address
Kernel_thread::init_workload_s0_stack()
{
  // push address of kernel info page to sigma0's stack
  Address sp = Kip::k()->sigma0_sp - sizeof(Mword);
  // assume we run in kdir 1:1 mapping
  *reinterpret_cast<Address*>(sp) = Kmem::virt_to_phys(Kip::k());
  return sp;
}

IMPLEMENTATION [ux,arm,ppc32,sparc]:

PRIVATE inline
Address
Kernel_thread::init_workload_s0_stack()
{ return Kip::k()->sigma0_sp; }

