INTERFACE:

#include "mapping_tree.h"
#include "kobject.h"
#include "kmem_slab.h"
#include "l4_types.h"
#include "prio_list.h"
#include "slab_cache.h"
#include "ref_obj.h"

class Ram_quota;

class Vm : public Kobject, public Ref_cnt_obj
{
  FIASCO_DECLARE_KOBJ();

  struct machine_state
    {
      Mword r[13];

      Mword sp_usr;
      Mword lr_usr;

      Mword sp_irq;
      Mword lr_irq;
      Mword spsr_irq;

      Mword r_fiq[5]; // r8 - r12
      Mword sp_fiq;
      Mword lr_fiq;
      Mword spsr_fiq;

      Mword sp_abt;
      Mword lr_abt;
      Mword spsr_abt;

      Mword sp_und;
      Mword lr_und;
      Mword spsr_und;

      Mword sp_svc;
      Mword lr_svc;
      Mword spsr_svc;
      
      Mword pc;
      Mword cpsr;

      Mword pending_events;
      
      Mword cp15_ttbr0;
      Mword cp15_ttbr1;
      Mword cp15_ttbcr;
      Mword cp15_vector_base;
      Mword cp15_dfsr;
      Mword cp15_dfar;
      Mword cp15_ifsr;
      Mword cp15_ifar;
      Mword cp15_control;
      Mword cp15_prim_region_remap;
      Mword cp15_norm_region_remap;
      Mword cp15_cid;
      Mword cp15_tls[3];
      Mword cp10_fpexc;

      Mword pfs;
      Mword pfa;
      Mword exit_reason;
    };

private:
  typedef Slab_cache Allocator;

  Mword *_state;
  Space *_space; // space the state is contained in
};

//-----------------------------------------------------------------------------
INTERFACE [debug]:

#include "tb_entry.h"

EXTENSION class Vm
{
public:
  struct Vm_log : public Tb_entry
  {
    bool is_entry;
    Mword pc;
    Mword cpsr;
    Mword exit_reason;
    Mword pending_events;
    Mword r0;
    Mword r1;
    unsigned print(int maxlen, char *buf) const;
    unsigned vm_entry_log_fmt(int maxlen, char *buf) const;
    unsigned vm_exit_log_fmt(int maxlen, char *buf) const;
 };
};

//-----------------------------------------------------------------------------

IMPLEMENTATION:

#include "cpu.h"
#include "cpu_lock.h"
#include "entry_frame.h"
#include "ipc_timeout.h"
#include "logdefs.h"
#include "mem_space.h"
#include "thread_state.h"
#include "timer.h"
#include "ref_ptr.h"

FIASCO_DEFINE_KOBJ(Vm);

PUBLIC
static
Vm *
Vm::create(Ram_quota *quota)
{
  if (void *a = allocator()->q_alloc(quota))
    {
      Vm *vm = new (a) Vm;
      return vm;
    }

  return 0;
}

PUBLIC
Vm::Vm() : _state(0), _space(0)
{ inc_ref(); }

PUBLIC virtual
bool
Vm::put()
{ return dec_ref() == 0; }

PUBLIC
inline
Vm::machine_state *
Vm::state()
{ return reinterpret_cast<machine_state *>(_state); };

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && tz]:

#include "thread.h"

void
Vm::run(Syscall_frame *f, Utcb *utcb)
{
  assert(cpu_lock.test());
  Ref_ptr<Vm> ref_ptr(this);

  L4_msg_tag const &tag = f->tag();

  if (tag.words() != 3)
    {
      WARN("tz: Invalid message length\n");
      f->tag(commit_result(-L4_err::EInval));
      return;
    }

  // FIXME: use a send item (fpage) for vm_state. this implementation is wrong!
  L4_fpage state_fpage(utcb->values[1]);

  if (!state_fpage.is_mempage()
      || state_fpage.order() != 12)
    {
      WARN("tz: Fpage invalid\n");
      f->tag(commit_result(-L4_err::EInval));
      return;
    }

  _state = (Mword *)(Virt_addr(state_fpage.mem_address()).value());

    {
      bool resident;
      Mem_space::Phys_addr phys;
      Mem_space::Size size;
      unsigned int attribs;

      _space = current()->space();
      assert_opt (_space);
      Mem_space *const curr_mem_space = _space;
      resident = curr_mem_space->v_lookup(Virt_addr(_state), &phys, &size, &attribs);

      if (!resident)
	{
	  WARN("tz: Vm_state not mapped\n");
	  f->tag(commit_result(-L4_err::EInval));
	  return;
	}
    }

  // set the temporary label for this VM
  Mword label = f->from_spec();

  while (true)
    {
      Proc::preemption_point();

      if (current_thread()->sender_list()->first())
	{
	  current_thread()->do_ipc(L4_msg_tag(), 0, 0, true, 0,
	      L4_timeout_pair(L4_timeout::Zero, L4_timeout::Zero), f, 7);
	  if (EXPECT_TRUE(!f->tag().has_error()))
	    return;
	  WARN("tz: Receive event failed\n");
	}

      log_vm(this, 1);

      if (!get_fpu())
	{
	  f->tag(commit_result(-L4_err::EInval));
	  return;
	}

      Cpu::cpus.cpu(current()->cpu()).tz_switch_to_ns(_state);

      assert(cpu_lock.test());

      log_vm(this, 0);

      if ((state()->exit_reason != 1) || 
	 ((state()->exit_reason == 1) &&
	  ((state()->r[0] & 0xffff0000) == 0xffff0000)))
	continue;
      else
	break;
    }

  // set label as return value for this vm
  f->from(label);
  f->tag(L4_msg_tag(L4_msg_tag::Label_task, 0, 0, 0));
  return;
}

IMPLEMENTATION [arm && tz && fpu]:

PUBLIC
bool
Vm::get_fpu()
{
  if (!(current()->state() & Thread_fpu_owner))
    {
      if (!current_thread()->switchin_fpu())
        {
          printf("tz: switchin_fpu failed\n");
          return false;
        }
    }
  return true;
}

// --------------------------------------------------------------------------
IMPLEMENTATION [arm && tz && !fpu]:

PUBLIC
bool
Vm::get_fpu()
{ return true; }

IMPLEMENTATION [arm && !tz]:

PUBLIC
L4_msg_tag
Vm::run(Utcb *u)
{
  return L4_msg_tag(0, 0, 0, 0);
}

// --------------------------------------------------------------------------
IMPLEMENTATION:

PUBLIC
void *
Vm::operator new (size_t, void *p) throw()
{ return p; }

PUBLIC
void
Vm::operator delete (void *_l)
{
  Vm *l = reinterpret_cast<Vm*>(_l);
  allocator()->free(l);
}

static Kmem_slab_t<Vm> _vm_allocator("Vm");

PRIVATE static
Vm::Allocator *
Vm::allocator()
{ return &_vm_allocator; }

PUBLIC
void
Vm::invoke(L4_obj_ref, Mword, Syscall_frame *f, Utcb *u)
{
  switch (f->tag().proto())
    {
    case 0:
    case L4_msg_tag::Label_task:
      run(nonull_static_cast<Syscall_frame*>(f), u);
      return;
    default:
      break;
    }

  f->tag(L4_msg_tag(0,0,0,-L4_err::EInval));
}

// --------------------------------------------------------------------------
IMPLEMENTATION [debug]:

#include "jdb.h"

PRIVATE
Mword
Vm::jdb_get(Mword *state_ptr)
{
  Mword v = ~0UL;
  Jdb::peek(state_ptr, _space, v);
  return v;
}

PUBLIC
void
Vm::dump_machine_state()
{
  machine_state *s = reinterpret_cast<machine_state*>(_state);
  printf("pc: %08lx  cpsr: %08lx exit_reason:%ld \n",
      jdb_get(&s->pc), jdb_get(&s->cpsr), jdb_get(&s->exit_reason));
  printf("r0: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
      jdb_get(&s->r[0]), jdb_get(&s->r[1]), jdb_get(&s->r[2]), jdb_get(&s->r[3]),
      jdb_get(&s->r[4]), jdb_get(&s->r[5]), jdb_get(&s->r[6]), jdb_get(&s->r[7]));
  printf("r8: %08lx %08lx %08lx %08lx %08lx\n",
      jdb_get(&s->r[8]), jdb_get(&s->r[9]), jdb_get(&s->r[10]), jdb_get(&s->r[11]),
      jdb_get(&s->r[12]));

  printf("usr: sp %08lx lr %08lx\n",
      jdb_get(&s->sp_usr), jdb_get(&s->lr_usr));
  printf("irq: sp %08lx lr %08lx psr %08lx\n",
      jdb_get(&s->sp_irq), jdb_get(&s->lr_irq), jdb_get(&s->spsr_irq));
  printf("fiq: sp %08lx lr %08lx psr %08lx\n",
      jdb_get(&s->sp_fiq), jdb_get(&s->lr_fiq), jdb_get(&s->spsr_fiq));
  printf("r8: %08lx %08lx %08lx %08lx %08lx\n",
      jdb_get(&s->r_fiq[0]), jdb_get(&s->r_fiq[1]), jdb_get(&s->r_fiq[2]),
      jdb_get(&s->r_fiq[3]), jdb_get(&s->r_fiq[4]));

  printf("abt: sp %08lx lr %08lx psr %08lx\n",
      jdb_get(&s->sp_abt), jdb_get(&s->lr_abt), jdb_get(&s->spsr_abt));
  printf("und: sp %08lx lr %08lx psr %08lx\n",
      jdb_get(&s->sp_und), jdb_get(&s->lr_und), jdb_get(&s->spsr_und));
  printf("svc: sp %08lx lr %08lx psr %08lx\n",
      jdb_get(&s->sp_svc), jdb_get(&s->lr_svc), jdb_get(&s->spsr_svc));
  printf("cp15_sctlr:%08lx\n", jdb_get(&s->cp15_control));
  printf("cp15_ttbr0:%08lx\n", jdb_get(&s->cp15_ttbr0));
  printf("cp15_ttbr1:%08lx\n", jdb_get(&s->cp15_ttbr1));
  printf("cp15_ttbcr:%08lx\n", jdb_get(&s->cp15_ttbcr));
  printf("dfar: %08lx dfsr: %08lx ifar: %08lx ifsr: %08lx\n",
      jdb_get(&s->cp15_dfar), jdb_get(&s->cp15_dfsr), jdb_get(&s->cp15_ifar),
      jdb_get(&s->cp15_ifsr));
}

PUBLIC
int
Vm::show_short(char *buf, int max)
{
  return snprintf(buf, max, " utcb:%lx pc:%lx ", (Mword)_state, (Mword)jdb_get(&state()->pc));
}

IMPLEMENT
unsigned
Vm::Vm_log::print(int maxlen, char *buf) const
{
  if (is_entry)
    return vm_entry_log_fmt(maxlen, buf);
  else
    return vm_exit_log_fmt(maxlen, buf);
}

IMPLEMENT
unsigned
Vm::Vm_log::vm_entry_log_fmt(int maxlen, char *buf) const
{
  if (r0 == 0x1110)
    return snprintf(buf, maxlen, "entry: pc:%08lx/%03lx intack irq: %lx", pc, pending_events, r1);

  return snprintf(buf, maxlen, "entry: pc:%08lx/%03lx r0:%lx", pc, pending_events, r0);
}

IMPLEMENT
unsigned
Vm::Vm_log::vm_exit_log_fmt(int maxlen, char *buf) const
{
  if ((r0 & 0xffff0000) == 0xffff0000)
    return snprintf(buf, maxlen, "=====: pc:%08lx/%03lx [%04lx]", pc, pending_events, r0 & 0xffff);
  if (r0 == 0x1105)
    return snprintf(buf, maxlen, "exit:  pc:%08lx/%03lx enable irq: %lx", pc, pending_events, r1);
  if (r0 == 0x1109)
    return snprintf(buf, maxlen, "exit:  pc:%08lx/%03lx disable irq: %lx", pc, pending_events, r1);
  if (r0 == 0x1110)
    return snprintf(buf, maxlen, "exit:  pc:%08lx/%03lx intack", pc, pending_events);
  if (r0 == 0x1115)
    return snprintf(buf, maxlen, "exit:  pc:%08lx/%03lx send ipi:%lx", pc, pending_events, r1);

  return snprintf(buf, maxlen, "exit:  pc:%08lx/%03lx r0:%lx", pc, pending_events, r0);
}

PUBLIC static inline
void
Vm::log_vm(Vm *vm, bool is_entry)
{
  if (vm->state()->exit_reason != 1)
      return;
  if ((vm->state()->r[0] & 0xf000) == 0x7000)
    return;
  if ((is_entry && (vm->state()->r[0] & 0xffff0000) == 0xffff0000))
    return;
  LOG_TRACE("VM entry/entry", "VM", current(), Vm_log,
      l->is_entry = is_entry;
      l->pc = vm->state()->pc;
      l->cpsr = vm->state()->cpsr;
      l->exit_reason = vm->state()->exit_reason;
      l->pending_events = vm->state()->pending_events;
      l->r0 = vm->state()->r[0];
      l->r1 = vm->state()->r[1];
  );
}

// --------------------------------------------------------------------------
IMPLEMENTATION [!debug]:

PUBLIC static inline
void
Vm::log_vm(Vm *, bool)
{}
