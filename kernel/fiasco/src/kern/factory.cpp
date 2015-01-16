INTERFACE:

#include "fiasco_defs.h"
#include "ram_quota.h"
#include "slab_cache.h"
#include "kobject_helper.h"

class Factory : public Ram_quota, public Kobject_h<Factory>
{
  FIASCO_DECLARE_KOBJ();

private:
  typedef Slab_cache Self_alloc;
};

//---------------------------------------------------------------------------
IMPLEMENTATION:

#include "ipc_gate.h"
#include "kmem_slab.h"
#include "task.h"
#include "thread_object.h"
#include "static_init.h"
#include "l4_buf_iter.h"
#include "l4_types.h"
#include "irq.h"
#include "map_util.h"
#include "logdefs.h"
#include "entry_frame.h"

static Factory _root_factory INIT_PRIORITY(ROOT_FACTORY_INIT_PRIO);
FIASCO_DEFINE_KOBJ(Factory);

PUBLIC inline
Factory::Factory()
  : Ram_quota()
{}

PRIVATE inline
Factory::Factory(Ram_quota *q, unsigned long max)
  : Ram_quota(q, max)
{}


static Kmem_slab_t<Factory> _factory_allocator("Factory");

PRIVATE static
Factory::Self_alloc *
Factory::allocator()
{ return &_factory_allocator; }

PUBLIC static inline
Factory * FIASCO_PURE
Factory::root()
{ return nonull_static_cast<Factory*>(Ram_quota::root); }


PRIVATE
Factory *
Factory::create_factory(unsigned long max)
{
  Auto_quota<Ram_quota> q(this, sizeof(Factory) + max);
  if (EXPECT_FALSE(!q))
    return 0;

  void *nq = allocator()->alloc();
  if (EXPECT_FALSE(!nq))
    return 0;

  q.release();
  return new (nq) Factory(this, max);
}

PUBLIC
void Factory::operator delete (void *_f)
{
  Factory *f = (Factory*)_f;
  LOG_TRACE("Factory delete", "fa del", ::current(), Tb_entry_empty, {});

  if (!f->parent())
    return;

  Ram_quota *p = f->parent();

  allocator()->free(f);
  if (p)
    p->free(sizeof(Factory) + f->limit());
}

PRIVATE
L4_msg_tag
Factory::map_obj(Kobject_iface *o, Cap_index cap, Task *c_space,
                 Obj_space *o_space, Utcb const *utcb)
{
  // must be before the lock guard
  Reap_list rl;

  auto space_lock_guard = lock_guard_dont_lock(c_space->existence_lock);

  // We take the existence_lock for syncronizing maps...
  // This is kind of coarse grained
  // try_lock fails if the lock is neither locked nor unlocked
  if (!space_lock_guard.check_and_lock(&c_space->existence_lock))
    {
      delete o;
      return commit_error(utcb, L4_error(L4_error::Overflow, L4_error::Rcv));
    }

  if (!map(o, o_space, c_space, cap, rl.list()))
    {
      delete o;
      return commit_result(-L4_err::ENomem);
    }

  // return a tag with one typed item for the returned capability
  return commit_result(0, 0, 1);
}



PRIVATE inline NOEXPORT
Kobject_iface *
Factory::new_factory(Utcb const *u, int *)
{
  return create_factory(u->values[2]);
}


PRIVATE inline NOEXPORT
Kobject_iface *
Factory::new_task(Utcb const *u, int *)
{
  L4_fpage utcb_area(0);
  utcb_area = L4_fpage(u->values[2]);

  if (Task *new_t = Task::create<Task>(this, utcb_area))
    return new_t;

  return 0;
}

PRIVATE inline NOEXPORT
Kobject_iface *
Factory::new_thread(Utcb const *, int *)
{ return new (this) Thread_object(); }

PRIVATE inline NOEXPORT
Kobject_iface *
Factory::new_gate(L4_msg_tag const &tag, Utcb const *utcb, Obj_space *o_space,
                  int *err)
{
  L4_snd_item_iter snd_items(utcb, tag.words());
  Thread *thread = 0;

  if (tag.items() && snd_items.next())
    {
      L4_fpage bind_thread(snd_items.get()->d);
      *err = L4_err::EInval;
      if (EXPECT_FALSE(!bind_thread.is_objpage()))
	return 0;

      L4_fpage::Rights thread_rights = L4_fpage::Rights(0);
      thread = Kobject::dcast<Thread_object*>(o_space->lookup_local(bind_thread.obj_index(), &thread_rights));

      *err = L4_err::EPerm;
      if (EXPECT_FALSE(!(thread_rights & L4_fpage::Rights::W())))
	return 0;
    }

  *err = L4_err::ENomem;
  return static_cast<Ipc_gate_ctl*>(Ipc_gate::create(this, thread, utcb->values[2]));
}

PRIVATE inline NOEXPORT
Kobject_iface *
Factory::new_irq(unsigned w, Utcb const *utcb, int *)
{
  if (w >= 3 && utcb->values[2])
    return Irq::allocate<Irq_muxer>(this);
  else
    return Irq::allocate<Irq_sender>(this);
}

PUBLIC
L4_msg_tag
Factory::kinvoke(L4_obj_ref ref, L4_fpage::Rights rights, Syscall_frame *f,
                 Utcb const *utcb, Utcb *)
{
  register Context *const c_thread = ::current();
  register Task *const c_space = static_cast<Task*>(c_thread->space());

  if (EXPECT_FALSE(f->tag().proto() != L4_msg_tag::Label_factory))
    return commit_result(-L4_err::EBadproto);

  if (EXPECT_FALSE(!(rights & L4_fpage::Rights::CS())))
    return commit_result(-L4_err::EPerm);

  if (EXPECT_FALSE(!ref.have_recv()))
    return commit_result(0);

  if (f->tag().words() < 1)
    return commit_result(-L4_err::EInval);

  L4_fpage buffer(0);

    {
      L4_buf_iter buf(utcb, utcb->buf_desc.obj());
      L4_buf_iter::Item const *const b = buf.get();
      if (EXPECT_FALSE(b->b.is_void()
	    || b->b.type() != L4_msg_item::Map))
	return commit_error(utcb, L4_error(L4_error::Overflow, L4_error::Rcv));

      buffer = L4_fpage(b->d);
    }

  if (EXPECT_FALSE(!buffer.is_objpage()))
    return commit_error(utcb, L4_error(L4_error::Overflow, L4_error::Rcv));

  Kobject_iface *new_o;
  int err = L4_err::ENomem;

  auto cpu_lock_guard = lock_guard<Lock_guard_inverse_policy>(cpu_lock);

  switch ((long)utcb->values[0])
    {
    case 0:  // new IPC Gate
      new_o = new_gate(f->tag(), utcb, c_space, &err);
      break;

    case L4_msg_tag::Label_factory:
      new_o = new_factory(utcb, &err);
      break;

    case L4_msg_tag::Label_task:
      new_o = new_task(utcb, &err);
      break;

    case L4_msg_tag::Label_thread:
      new_o = new_thread(utcb, &err);
      break;

    case L4_msg_tag::Label_irq:
      new_o = new_irq(f->tag().words(), utcb, &err);
      break;

    case L4_msg_tag::Label_vm:
      new_o = new_vm(utcb, &err);
      break;

    default:
      return commit_result(-L4_err::ENodev);
    }

  LOG_TRACE("Kobject create", "new", ::current(), Log_entry,
    l->op = utcb->values[0];
    l->buffer = buffer.obj_index();
    l->id = dbg_info()->dbg_id();
    l->ram = current();
    l->newo = new_o ? new_o->dbg_info()->dbg_id() : ~0);

  if (new_o)
    return map_obj(new_o, buffer.obj_index(), c_space, c_space, utcb);
  else
    return commit_result(-err);

}


//----------------------------------------------------------------------------
IMPLEMENTATION [svm || vmx || arm_em_tz]:

#include "vm_factory.h"
#include "vm.h"

PRIVATE inline NOEXPORT
Kobject_iface *
Factory::new_vm(Utcb const *, int *err)
{
  if (Vm *new_t = Vm_factory::create(this, err))
    return new_t;

  return 0;
}

//----------------------------------------------------------------------------
IMPLEMENTATION [!svm && !vmx && !arm_em_tz]:

PRIVATE inline NOEXPORT
Kobject_iface *
Factory::new_vm(Utcb const *, int *err)
{
  *err = L4_err::ENodev;
  return 0;
}

// ------------------------------------------------------------------------
INTERFACE [debug]:

#include "tb_entry.h"

EXTENSION class Factory
{
private:
  struct Log_entry : public Tb_entry
  {
    Smword op;
    Cap_index buffer;
    Mword id;
    Mword ram;
    Mword newo;
    void print(String_buffer *buf) const;
  };
};

// ------------------------------------------------------------------------
IMPLEMENTATION [debug]:

#include "string_buffer.h"

IMPLEMENT
void
Factory::Log_entry::print(String_buffer *buf) const
{
  static char const *const ops[] =
  { /*   0 */ "gate", "irq", 0, 0, 0, 0, 0, 0,
    /*  -8 */ 0, 0, 0, "task", "thread", 0, 0, "factory",
    /* -16 */ "vm", 0, 0, 0, "sem" }; 
  char const *_op = -op <= (int)(sizeof(ops)/sizeof(ops[0]))
    ? ops[-op] : "invalid op";
  if (!_op)
    _op = "(nan)";

  buf->printf("factory=%lx [%s] new=%lx cap=[C:%lx] ram=%lx",
              id, _op, newo, cxx::int_value<Cap_index>(buffer), ram);
}
