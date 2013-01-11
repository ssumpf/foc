INTERFACE:

#include "l4_types.h"
#include "config.h"
#include "continuation.h"
#include "helping_lock.h"
#include "kobject.h"
#include "mem_layout.h"
#include "member_offs.h"
#include "receiver.h"
#include "ref_obj.h"
#include "sender.h"
#include "spin_lock.h"
#include "thread_lock.h"

class Return_frame;
class Syscall_frame;
class Task;
class Thread;
class Vcpu_state;
class Irq_base;

typedef Context_ptr_base<Thread> Thread_ptr;


/** A thread.  This class is the driver class for most kernel functionality.
 */
class Thread :
  public Receiver,
  public Sender,
  public Kobject
{
  MEMBER_OFFSET();
  FIASCO_DECLARE_KOBJ();

  friend class Jdb;
  friend class Jdb_bt;
  friend class Jdb_tcb;
  friend class Jdb_thread;
  friend class Jdb_thread_list;
  friend class Jdb_list_threads;
  friend class Jdb_list_timeouts;
  friend class Jdb_tbuf_show;

public:
  enum Context_mode_kernel { Kernel = 0 };
  enum Operation
  {
    Opcode_mask = 0xffff,
    Op_control = 0,
    Op_ex_regs = 1,
    Op_switch  = 2,
    Op_stats   = 3,
    Op_vcpu_resume = 4,
    Op_register_del_irq = 5,
    Op_modify_senders = 6,
    Op_vcpu_control= 7,
    Op_gdt_x86 = 0x10,
    Op_set_fs_amd64 = 0x12,
  };

  enum Control_flags
  {
    Ctl_set_pager       = 0x0010000,
    Ctl_bind_task       = 0x0200000,
    Ctl_alien_thread    = 0x0400000,
    Ctl_ux_native       = 0x0800000,
    Ctl_set_exc_handler = 0x1000000,
  };

  enum Ex_regs_flags
  {
    Exr_cancel            = 0x10000,
    Exr_trigger_exception = 0x20000,
  };

  enum Vcpu_ctl_flags
  {
    Vcpu_ctl_extendet_vcpu = 0x10000,
  };


  class Dbg_stack
  {
  public:
    enum { Stack_size = Config::PAGE_SIZE };
    void *stack_top;
    Dbg_stack();
  };

  static Per_cpu<Dbg_stack> dbg_stack;

public:
  typedef void (Utcb_copy_func)(Thread *sender, Thread *receiver);

  /**
   * Constructor.
   *
   * @param task the task the thread should reside in.
   * @param id user-visible thread ID of the sender.
   * @param init_prio initial priority.
   * @param mcp maximum controlled priority.
   *
   * @post state() != Thread_invalid.
   */
  Thread();

  int handle_page_fault (Address pfa, Mword error, Mword pc,
      Return_frame *regs);

private:
  struct Migration_helper_info
  {
    Migration_info inf;
    Thread *victim;
  };

  Thread(const Thread&);	///< Default copy constructor is undefined
  void *operator new(size_t);	///< Default new operator undefined

  bool handle_sigma0_page_fault (Address pfa);

  /**
   * Return to user.
   *
   * This function is the default routine run if a newly
   * initialized context is being switch_exec()'ed.
   */
  static void user_invoke();

public:
  static bool pagein_tcb_request(Return_frame *regs);

  inline Mword user_ip() const;
  inline void user_ip(Mword);

  inline Mword user_sp() const;
  inline void user_sp(Mword);

  inline Mword user_flags() const;

  /** nesting level in debugger (always critical) if >1 */
  static Per_cpu<unsigned long> nested_trap_recover;
  static void handle_remote_requests_irq() asm ("handle_remote_cpu_requests");
  static void handle_global_remote_requests_irq() asm ("ipi_remote_call");

protected:
  explicit Thread(Context_mode_kernel);

  // Another critical TCB cache line:
  Thread_lock  _thread_lock;

  // More ipc state
  Thread_ptr _pager;
  Thread_ptr _exc_handler;

protected:
  Ram_quota *_quota;
  Irq_base *_del_observer;

  // debugging stuff
  unsigned _magic;
  static const unsigned magic = 0xf001c001;
};


IMPLEMENTATION:

#include <cassert>
#include <cstdlib>		// panic()
#include <cstring>
#include "atomic.h"
#include "entry_frame.h"
#include "fpu_alloc.h"
#include "globals.h"
#include "irq_chip.h"
#include "kdb_ke.h"
#include "kernel_task.h"
#include "kmem_alloc.h"
#include "logdefs.h"
#include "map_util.h"
#include "ram_quota.h"
#include "sched_context.h"
#include "space.h"
#include "std_macros.h"
#include "task.h"
#include "thread_state.h"
#include "timeout.h"

FIASCO_DEFINE_KOBJ(Thread);

DEFINE_PER_CPU Per_cpu<unsigned long> Thread::nested_trap_recover;


IMPLEMENT
Thread::Dbg_stack::Dbg_stack()
{
  stack_top = Kmem_alloc::allocator()->unaligned_alloc(Stack_size); 
  if (stack_top)
    stack_top = (char *)stack_top + Stack_size;
  //printf("JDB STACK start= %p - %p\n", (char *)stack_top - Stack_size, (char *)stack_top);
}


PUBLIC inline NEEDS[Thread::thread_lock]
void
Thread::kill_lock()
{ thread_lock()->lock(); }


PUBLIC inline
void *
Thread::operator new(size_t, Ram_quota *q) throw ()
{
  void *t = Kmem_alloc::allocator()->q_unaligned_alloc(q, Thread::Size);
  if (t)
    {
      memset(t, 0, sizeof(Thread));
      reinterpret_cast<Thread*>(t)->_quota = q;
    }
  return t;
}

PUBLIC
bool
Thread::bind(Task *t, User<Utcb>::Ptr utcb)
{
  // _utcb == 0 for all kernel threads
  Space::Ku_mem const *u = t->find_ku_mem(utcb, sizeof(Utcb));

  // kernel thread?
  if (EXPECT_FALSE(utcb && !u))
    return false;

  Lock_guard<decltype(*_space.lock())> guard(_space.lock());
  if (_space.space() != Kernel_task::kernel_task())
    return false;

  _space.space(t);
  t->inc_ref();

  if (u)
    {
      _utcb.set(utcb, u->kern_addr(utcb));
      arch_setup_utcb_ptr();
    }

  return true;
}


PUBLIC inline NEEDS["kdb_ke.h", "kernel_task.h", "cpu_lock.h", "space.h"]
bool
Thread::unbind()
{
  Task *old;

    {
      Lock_guard<decltype(*_space.lock())> guard(_space.lock());

      if (_space.space() == Kernel_task::kernel_task())
	return true;

      old = static_cast<Task*>(_space.space());
      _space.space(Kernel_task::kernel_task());

      // switch to a safe page table
      if (Mem_space::current_mem_space(current_cpu()) == old)
        Kernel_task::kernel_task()->switchin_context(old);

      if (old->dec_ref())
	old = 0;
    }

  if (old)
    {
      current()->rcu_wait();
      delete old;
    }

  return true;
}

/** Cut-down version of Thread constructor; only for kernel threads
    Do only what's necessary to get a kernel thread started --
    skip all fancy stuff, no locking is necessary.
    @param task the address space
    @param id user-visible thread ID of the sender
 */
IMPLEMENT inline
Thread::Thread(Context_mode_kernel)
  : Receiver(), Sender(), _del_observer(0), _magic(magic)
{
  *reinterpret_cast<void(**)()>(--_kernel_sp) = user_invoke;

  inc_ref();
  _space.space(Kernel_task::kernel_task());

  if (Config::Stack_depth)
    std::memset((char*)this + sizeof(Thread), '5',
		Thread::Size-sizeof(Thread)-64);
}


/** Destructor.  Reestablish the Context constructor's precondition.
    @pre current() == thread_lock()->lock_owner()
         && state() == Thread_dead
    @pre lock_cnt() == 0
    @post (_kernel_sp == 0)  &&  (* (stack end) == 0)  &&  !exists()
 */
PUBLIC virtual
Thread::~Thread()		// To be called in locked state.
{

  unsigned long *init_sp = reinterpret_cast<unsigned long*>
    (reinterpret_cast<unsigned long>(this) + Size - sizeof(Entry_frame));


  _kernel_sp = 0;
  *--init_sp = 0;
  Fpu_alloc::free_state(fpu_state());
  _state = Thread_invalid;
}


// IPC-gate deletion stuff ------------------------------------

/**
 * Fake IRQ Chip class for IPC-gate-delete notifications.
 * This chip uses the IRQ number as thread pointer and implements
 * the bind and unbind functionality.
 */
class Del_irq_chip : public Irq_chip_soft
{
public:
  static Del_irq_chip chip;
};

Del_irq_chip Del_irq_chip::chip;

PUBLIC static inline
Thread *Del_irq_chip::thread(Mword pin)
{ return (Thread*)pin; }

PUBLIC static inline
Mword Del_irq_chip::pin(Thread *t)
{ return (Mword)t; }

PUBLIC inline
void
Del_irq_chip::unbind(Irq_base *irq)
{ thread(irq->pin())->remove_delete_irq(); }


PUBLIC inline NEEDS["irq_chip.h"]
void
Thread::ipc_gate_deleted(Mword id)
{
  (void) id;
  Lock_guard<Cpu_lock> g(&cpu_lock);
  if (_del_observer)
    _del_observer->hit(0);
}

PUBLIC
void
Thread::register_delete_irq(Irq_base *irq)
{
  irq->unbind();
  Del_irq_chip::chip.bind(irq, (Mword)this);
  _del_observer = irq;
}

PUBLIC
void
Thread::remove_delete_irq()
{
  if (!_del_observer)
    return;

  Irq_base *tmp = _del_observer;
  _del_observer = 0;
  tmp->unbind();
}

// end of: IPC-gate deletion stuff -------------------------------


/** Currently executing thread.
    @return currently executing thread.
 */
inline
Thread*
current_thread()
{ return nonull_static_cast<Thread*>(current()); }

PUBLIC inline
bool
Thread::exception_triggered() const
{ return _exc_cont.valid(); }

PUBLIC inline
bool
Thread::continuation_test_and_restore()
{
  bool v = _exc_cont.valid();
  if (v)
    _exc_cont.restore(regs());
  return v;
}

//
// state requests/manipulation
//


/** Thread lock.
    Overwrite Context's version of thread_lock() with a semantically
    equivalent, but more efficient version.
    @return lock used to synchronize accesses to the thread.
 */
PUBLIC inline
Thread_lock *
Thread::thread_lock()
{ return &_thread_lock; }


PUBLIC inline NEEDS ["config.h", "timeout.h"]
void
Thread::handle_timer_interrupt()
{
  unsigned _cpu = cpu(true);
  // XXX: This assumes periodic timers (i.e. bogus in one-shot mode)
  if (!Config::Fine_grained_cputime)
    consume_time(Config::Scheduler_granularity);

  bool resched = Rcu::do_pending_work(_cpu);

  // Check if we need to reschedule due to timeouts or wakeups
  if ((Timeout_q::timeout_queue.cpu(_cpu).do_timeouts() || resched)
      && !schedule_in_progress())
    {
      schedule();
      assert (timeslice_timeout.cpu(cpu(true))->is_set());	// Coma check
    }
}


PUBLIC
void
Thread::halt()
{
  // Cancel must be cleared on all kernel entry paths. See slowtraps for
  // why we delay doing it until here.
  state_del(Thread_cancel);

  // we haven't been re-initialized (cancel was not set) -- so sleep
  if (state_change_safely(~Thread_ready, Thread_cancel | Thread_dead))
    while (! (state() & Thread_ready))
      schedule();
}

PUBLIC static
void
Thread::halt_current()
{
  for (;;)
    {
      current_thread()->halt();
      kdb_ke("Thread not halted");
    }
}

PRIVATE static inline
void
Thread::user_invoke_generic()
{
  Context *const c = current();
  assert_kdb (c->state() & Thread_ready_mask);

  if (c->handle_drq() && !c->schedule_in_progress())
    c->schedule();

  // release CPU lock explicitly, because
  // * the context that switched to us holds the CPU lock
  // * we run on a newly-created stack without a CPU lock guard
  cpu_lock.clear();
}


PRIVATE static void
Thread::leave_and_kill_myself()
{
  current_thread()->do_kill();
#ifdef CONFIG_JDB
  WARN("dead thread scheduled: %lx\n", current_thread()->dbg_id());
#endif
  kdb_ke("DEAD SCHED");
}

PUBLIC static
unsigned
Thread::handle_kill_helper(Drq *src, Context *, void *)
{
  delete nonull_static_cast<Thread*>(src->context());
  return Drq::No_answer | Drq::Need_resched;
}


PRIVATE
bool
Thread::do_kill()
{
  Lock_guard<Thread_lock> guard(thread_lock());

  if (state() == Thread_invalid)
    return false;

  //
  // Kill this thread
  //

  // But first prevent it from being woken up by asynchronous events

  {
    Lock_guard <Cpu_lock> guard(&cpu_lock);

    // if IPC timeout active, reset it
    if (_timeout)
      _timeout->reset();

    // Switch to time-sharing mode
    set_mode(Sched_mode(0));

    // Switch to time-sharing scheduling context
    if (sched() != sched_context())
      switch_sched(sched_context());

    if (!current_sched() || current_sched()->context() == this)
      set_current_sched(current()->sched());
  }

  // if other threads want to send me IPC messages, abort these
  // operations
  {
    Lock_guard <Cpu_lock> guard(&cpu_lock);
    while (Sender *s = Sender::cast(sender_list()->first()))
      {
	s->sender_dequeue(sender_list());
	vcpu_update_state();
	s->ipc_receiver_aborted();
	Proc::preemption_point();
      }
  }

  // if engaged in IPC operation, stop it
  if (in_sender_list())
    {
      while (Locked_prio_list *q = wait_queue())
	{
	  Lock_guard<decltype(q->lock())> g(q->lock());
	  if (wait_queue() == q)
	    {
	      sender_dequeue(q);
	      set_wait_queue(0);
	      break;
	    }
	}
    }

  Context::do_kill();

  vcpu_update_state();

  unbind();
  vcpu_set_user_space(0);

  cpu_lock.lock();

  state_change_dirty(0, Thread_dead);

  // dequeue from system queues
  ready_dequeue();

  if (_del_observer)
    {
      _del_observer->unbind();
      _del_observer = 0;
    }

  if (dec_ref())
    while (1)
      {
	state_del_dirty(Thread_ready_mask);
	schedule();
	WARN("woken up dead thread %lx\n", dbg_id());
	kdb_ke("X");
      }

  rcu_wait();

  state_del_dirty(Thread_ready_mask);

  ready_dequeue();

  kernel_context_drq(handle_kill_helper, 0);
  kdb_ke("Im dead");
  return true;
}

PRIVATE static
unsigned
Thread::handle_remote_kill(Drq *, Context *self, void *)
{
  Thread *c = nonull_static_cast<Thread*>(self);
  c->state_add_dirty(Thread_cancel | Thread_ready);
  c->_exc_cont.restore(c->regs());
  c->do_trigger_exception(c->regs(), (void*)&Thread::leave_and_kill_myself);
  return 0;
}


PROTECTED
bool
Thread::kill()
{
  Lock_guard<Cpu_lock> guard(&cpu_lock);
  inc_ref();


  if (cpu() == current_cpu())
    {
      state_add_dirty(Thread_cancel | Thread_ready);
      sched()->deblock(cpu());
      _exc_cont.restore(regs()); // overwrite an already triggered exception
      do_trigger_exception(regs(), (void*)&Thread::leave_and_kill_myself);
//          current()->switch_exec (this, Helping);
      return true;
    }

  drq(Thread::handle_remote_kill, 0, 0, Drq::Any_ctxt);

  return true;
}


PUBLIC
void
Thread::set_sched_params(unsigned prio, Unsigned64 quantum)
{
  Sched_context *sc = sched_context();
  bool const change = prio != sc->prio()
                   || quantum != sc->quantum();
  bool const ready_queued = in_ready_list();

  if (!change && (ready_queued || this == current()))
    return;

  ready_dequeue();

  sc->set_prio(prio);
  sc->set_quantum(quantum);
  sc->replenish();

  if (sc == current_sched())
    set_current_sched(sc);

  if (state() & Thread_ready_mask)
    {
      if (this != current())
        ready_enqueue();
      else
        schedule();
    }
}

PUBLIC
long
Thread::control(Thread_ptr const &pager, Thread_ptr const &exc_handler)
{
  if (pager.is_valid())
    _pager = pager;

  if (exc_handler.is_valid())
    _exc_handler = exc_handler;

  return 0;
}

// used by UX only
PUBLIC static inline
bool
Thread::is_tcb_address(Address a)
{
  a &= ~(Thread::Size - 1);
  return reinterpret_cast<Thread *>(a)->_magic == magic;
}

PUBLIC static inline
void
Thread::assert_irq_entry()
{
  assert_kdb(current_thread()->schedule_in_progress()
             || current_thread()->state() & (Thread_ready_mask | Thread_drq_wait | Thread_waiting | Thread_ipc_transfer));
}



// ---------------------------------------------------------------------------

PUBLIC inline
bool
Thread::check_sys_ipc(unsigned flags, Thread **partner, Thread **sender,
                      bool *have_recv) const
{
  if (flags & L4_obj_ref::Ipc_recv)
    {
      *sender = flags & L4_obj_ref::Ipc_open_wait ? 0 : const_cast<Thread*>(this);
      *have_recv = true;
    }

  if (flags & L4_obj_ref::Ipc_send)
    *partner = const_cast<Thread*>(this);

  // FIXME: shall be removed flags == 0 is no-op
  if (!flags)
    {
      *sender = const_cast<Thread*>(this);
      *partner = const_cast<Thread*>(this);
      *have_recv = true;
    }

  return *have_recv || ((flags & L4_obj_ref::Ipc_send) && *partner);
}

PUBLIC static
unsigned
Thread::handle_migration_helper(Drq *, Context *, void *p)
{
  Migration_helper_info const *inf = (Migration_helper_info const *)p;
  return inf->victim->migration_helper(&inf->inf);
}


PRIVATE
void
Thread::do_migration()
{
  assert_kdb(cpu_lock.test());
  assert_kdb(current_cpu() == cpu(true));

  Migration_helper_info inf;

    {
      Lock_guard<decltype(_migration_rq.affinity_lock)>
	g(&_migration_rq.affinity_lock);
      inf.inf = _migration_rq.inf;
      _migration_rq.pending = false;
      _migration_rq.in_progress = true;
    }

  unsigned on_cpu = cpu();

  if (inf.inf.cpu == ~0U)
    {
      state_add_dirty(Thread_suspended);
      set_sched_params(0, 0);
      _migration_rq.in_progress = false;
      return;
    }

  state_del_dirty(Thread_suspended);

  if (inf.inf.cpu == on_cpu)
    {
      // stay here
      set_sched_params(inf.inf.prio, inf.inf.quantum);
      _migration_rq.in_progress = false;
      return;
    }

  // spill FPU state into memory before migration
  if (state() & Thread_fpu_owner)
    {
      if (current() != this)
	Fpu::enable();

      spill_fpu();
      Fpu::set_owner(on_cpu, 0);
      Fpu::disable();
    }


  // if we are in the middle of the scheduler, leave it now
  if (schedule_in_progress() == this)
    reset_schedule_in_progress();

  inf.victim = this;

  if (current() == this && Config::Max_num_cpus > 1)
    kernel_context_drq(handle_migration_helper, &inf);
  else
    migration_helper(&inf.inf);
}

PUBLIC
void
Thread::initiate_migration()
{ do_migration(); }

PUBLIC
void
Thread::finish_migration()
{ enqueue_timeout_again(); }


PUBLIC
void
Thread::migrate(Migration_info const &info)
{
  assert_kdb (cpu_lock.test());

  LOG_TRACE("Thread migration", "mig", this, __thread_migration_log_fmt,
      Migration_log *l = tbe->payload<Migration_log>();
      l->state = state();
      l->src_cpu = cpu();
      l->target_cpu = info.cpu;
      l->user_ip = regs()->ip();
  );

    {
      Lock_guard<decltype(_migration_rq.affinity_lock)>
	g(&_migration_rq.affinity_lock);
      _migration_rq.inf = info;
      _migration_rq.pending = true;
    }

  unsigned cpu = this->cpu();

  if (current_cpu() == cpu)
    {
      do_migration();
      return;
    }

  migrate_xcpu(cpu);
}


//---------------------------------------------------------------------------
IMPLEMENTATION [fpu && !ux]:

#include "fpu.h"
#include "fpu_alloc.h"
#include "fpu_state.h"

PUBLIC inline NEEDS ["fpu.h"]
void
Thread::spill_fpu()
{
  // If we own the FPU, we should never be getting an "FPU unavailable" trap
  assert_kdb (Fpu::owner(cpu()) == this);
  assert_kdb (state() & Thread_fpu_owner);
  assert_kdb (fpu_state());

  // Save the FPU state of the previous FPU owner (lazy) if applicable
  Fpu::save_state (fpu_state());
  state_del_dirty (Thread_fpu_owner);
}


/*
 * Handle FPU trap for this context. Assumes disabled interrupts
 */
PUBLIC inline NEEDS [Thread::spill_fpu, "fpu_alloc.h","fpu_state.h"]
int
Thread::switchin_fpu(bool alloc_new_fpu = true)
{
  unsigned cpu = this->cpu(true);

  if (state() & Thread_vcpu_fpu_disabled)
    return 0;

  // If we own the FPU, we should never be getting an "FPU unavailable" trap
  assert_kdb (Fpu::owner(cpu) != this);

  // Allocate FPU state slab if we didn't already have one
  if (!fpu_state()->state_buffer()
      && (EXPECT_FALSE((!alloc_new_fpu
                        || (state() & Thread_alien))
                       || !Fpu_alloc::alloc_state(_quota, fpu_state()))))
    return 0;

  // Enable the FPU before accessing it, otherwise recursive trap
  Fpu::enable();

  // Save the FPU state of the previous FPU owner (lazy) if applicable
  if (Fpu::owner(cpu))
    nonull_static_cast<Thread*>(Fpu::owner(cpu))->spill_fpu();

  // Become FPU owner and restore own FPU state
  Fpu::restore_state(fpu_state());

  state_add_dirty(Thread_fpu_owner);
  Fpu::set_owner(cpu, this);
  return 1;
}

PUBLIC inline NEEDS["fpu.h", "fpu_alloc.h"]
void
Thread::transfer_fpu(Thread *to)
{
  unsigned cpu = this->cpu();
  if (cpu != to->cpu())
    return;

  if (to->fpu_state()->state_buffer())
    Fpu_alloc::free_state(to->fpu_state());

  to->fpu_state()->state_buffer(fpu_state()->state_buffer());
  fpu_state()->state_buffer(0);

  assert (current() == this || current() == to);

  Fpu::disable(); // it will be reanabled in switch_fpu

  if (EXPECT_FALSE(Fpu::owner(cpu) == to))
    {
      assert_kdb (to->state() & Thread_fpu_owner);

      Fpu::set_owner(cpu, 0);
      to->state_del_dirty (Thread_fpu_owner);
    }
  else if (Fpu::owner(cpu) == this)
    {
      assert_kdb (state() & Thread_fpu_owner);

      state_del_dirty (Thread_fpu_owner);

      to->state_add_dirty (Thread_fpu_owner);
      Fpu::set_owner(cpu, to);
      if (EXPECT_FALSE(current() == to))
        Fpu::enable();
    }
}

//---------------------------------------------------------------------------
IMPLEMENTATION [!fpu]:

PUBLIC inline
int
Thread::switchin_fpu(bool alloc_new_fpu = true)
{
  (void)alloc_new_fpu;
  return 0;
}

PUBLIC inline
void
Thread::spill_fpu()
{}

//---------------------------------------------------------------------------
IMPLEMENTATION [!fpu || ux]:

PUBLIC inline
void
Thread::transfer_fpu(Thread *)
{}

//---------------------------------------------------------------------------
IMPLEMENTATION [!log]:

PUBLIC inline
unsigned Thread::sys_ipc_log(Syscall_frame *)
{ return 0; }

PUBLIC inline
unsigned Thread::sys_ipc_trace(Syscall_frame *)
{ return 0; }

static inline
void Thread::page_fault_log(Address, unsigned, unsigned)
{}

PUBLIC static inline
int Thread::log_page_fault()
{ return 0; }

PUBLIC inline
unsigned Thread::sys_fpage_unmap_log(Syscall_frame *)
{ return 0; }


// ----------------------------------------------------------------------------
IMPLEMENTATION [!mp]:


PRIVATE inline
unsigned
Thread::migration_helper(Migration_info const *inf)
{
  unsigned cpu = inf->cpu;
  //  LOG_MSG_3VAL(this, "MGi ", Mword(current()), (current_cpu() << 16) | cpu(), Context::current_sched());
  if (_timeout)
    _timeout->reset();
  ready_dequeue();

    {
      // Not sure if this can ever happen
      Sched_context *csc = Context::current_sched();
      if (!csc || csc->context() == this)
	Context::set_current_sched(current()->sched());
    }

  Sched_context *sc = sched_context();
  sc->set_prio(inf->prio);
  sc->set_quantum(inf->quantum);
  sc->replenish();
  set_sched(sc);

  if (drq_pending())
    state_add_dirty(Thread_drq_ready);

  set_cpu_of(this, cpu);
  return  Drq::No_answer | Drq::Need_resched;
}

PRIVATE inline
void
Thread::migrate_xcpu(unsigned cpu)
{
  (void)cpu;
  assert_kdb (false);
}


//----------------------------------------------------------------------------
INTERFACE [debug]:

EXTENSION class Thread
{
protected:
  struct Migration_log
  {
    Mword    state;
    Address  user_ip;
    unsigned src_cpu;
    unsigned target_cpu;

    static unsigned fmt(Tb_entry *, int, char *)
    asm ("__thread_migration_log_fmt");
  };
};


// ----------------------------------------------------------------------------
IMPLEMENTATION [mp]:

#include "ipi.h"

IMPLEMENT
void
Thread::handle_remote_requests_irq()
{
  assert_kdb (cpu_lock.test());
  // printf("CPU[%2u]: > RQ IPI (current=%p)\n", current_cpu(), current());
  Context *const c = current();
  Ipi::eoi(Ipi::Request, c->cpu());
  //LOG_MSG_3VAL(c, "ipi", c->cpu(), (Mword)c, c->drq_pending());
  Context *migration_q = 0;
  bool resched = _pending_rqq.cpu(c->cpu()).handle_requests(&migration_q);

  resched |= Rcu::do_pending_work(c->cpu());

  if (migration_q)
    static_cast<Thread*>(migration_q)->do_migration();

  if ((resched || c->handle_drq()) && !c->schedule_in_progress())
    {
      //LOG_MSG_3VAL(c, "ipis", 0, 0, 0);
      // printf("CPU[%2u]: RQ IPI sched %p\n", current_cpu(), current());
      c->schedule();
    }
  // printf("CPU[%2u]: < RQ IPI (current=%p)\n", current_cpu(), current());
}

IMPLEMENT
void
Thread::handle_global_remote_requests_irq()
{
  assert_kdb (cpu_lock.test());
  // printf("CPU[%2u]: > RQ IPI (current=%p)\n", current_cpu(), current());
  Ipi::eoi(Ipi::Global_request, current_cpu());
  Context::handle_global_requests();
}

PRIVATE inline
unsigned
Thread::migration_helper(Migration_info const *inf)
{
  // LOG_MSG_3VAL(this, "MGi ", Mword(current()), (current_cpu() << 16) | cpu(), 0);
  assert_kdb (cpu() == current_cpu());
  assert_kdb (current() != this);
  assert_kdb (cpu_lock.test());

  if (_timeout)
    _timeout->reset();
  ready_dequeue();

    {
      // Not sure if this can ever happen
      Sched_context *csc = Context::current_sched();
      if (!csc || csc->context() == this)
	Context::set_current_sched(current()->sched());
    }

  unsigned cpu = inf->cpu;

    {
      Queue &q = _pending_rqq.cpu(current_cpu());
      // The queue lock of the current CPU protects the cpu number in
      // the thread
      Lock_guard<Pending_rqq::Inner_lock> g(q.q_lock());

      // potentailly dequeue from our local queue
      if (_pending_rq.queued())
	check_kdb (q.dequeue(&_pending_rq, Queue_item::Ok));

      Sched_context *sc = sched_context();
      sc->set_prio(inf->prio);
      sc->set_quantum(inf->quantum);
      sc->replenish();
      set_sched(sc);

      if (drq_pending())
	state_add_dirty(Thread_drq_ready);

      Mem::mp_wmb();

      assert_kdb (!in_ready_list());

      set_cpu_of(this, cpu);
      // now we are migrated away fom current_cpu
    }

  bool ipi = true;

    {
      Queue &q = _pending_rqq.cpu(cpu);
      Lock_guard<Pending_rqq::Inner_lock> g(q.q_lock());

      // migrated meanwhile
      if (this->cpu() != cpu || _pending_rq.queued())
	return  Drq::No_answer | Drq::Need_resched;

      if (q.first())
	ipi = false;

      q.enqueue(&_pending_rq);
    }

  if (ipi)
    {
      //LOG_MSG_3VAL(this, "sipi", current_cpu(), cpu(), (Mword)current());
      Ipi::send(Ipi::Request, current_cpu(), cpu);
    }

  return  Drq::No_answer | Drq::Need_resched;
}

PRIVATE inline
void
Thread::migrate_xcpu(unsigned cpu)
{
  bool ipi = true;

    {
      Queue &q = Context::_pending_rqq.cpu(cpu);
      Lock_guard<Pending_rqq::Inner_lock> g(q.q_lock());

      // already migrated
      if (cpu != this->cpu())
	return;

      if (q.first())
	ipi = false;

      if (!_pending_rq.queued())
	q.enqueue(&_pending_rq);
      else
	ipi = false;
    }

  if (ipi)
    Ipi::send(Ipi::Request, current_cpu(), cpu);
}

//----------------------------------------------------------------------------
IMPLEMENTATION [debug]:

IMPLEMENT
unsigned
Thread::Migration_log::fmt(Tb_entry *e, int maxlen, char *buf)
{
  Migration_log *l = e->payload<Migration_log>();
  return snprintf(buf, maxlen, "migrate from %u to %u (state=%lx user ip=%lx)",
      l->src_cpu, l->target_cpu, l->state, l->user_ip);
}

