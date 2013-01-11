INTERFACE:

#include "queue.h"

class Mp_lock
{
public:
  enum Status { Not_locked, Locked, Invalid };

private:
  /** queue of blocked threads */
  Queue _q;
};


//----------------------------------------------------------------------------
IMPLEMENTATION:

#include "context.h"
#include "cpu_lock.h"
#include "globals.h"
#include "kdb_ke.h"
#include "lock_guard.h"
#include "thread_state.h"
//#include "logdefs.h"

PUBLIC inline NEEDS["context.h", "cpu_lock.h", "globals.h",
                    "kdb_ke.h", "lock_guard.h", "thread_state.h"]
Mp_lock::Status
Mp_lock::test_and_set()
{
  //assert_kdb (cpu_lock.test());

  Context *const c = current();

  Lock_guard<Cpu_lock> g(&cpu_lock);

    {
      Lock_guard<Queue::Inner_lock> guard(_q.q_lock());
      if (EXPECT_FALSE(_q.invalid()))
	return Invalid;

      if (!_q.blocked())
	{
	  // lock was free, take it
	  _q.block();
	  return Not_locked;
	}

      _q.enqueue(c->queue_item());
    }
  // LOG_MSG_3VAL(c, "block", (Mword)this, current_cpu(), *((Mword*)this));
  assert_kdb (!(c->state() & Thread_drq_ready));
  while (1)
    {
      c->state_change_dirty(~Thread_ready, Thread_waiting);
      c->schedule();

      if (!c->queue_item()->queued())
	break;
    }

  c->state_del_dirty(Thread_waiting);

  // LOG_MSG_3VAL(c, "woke", (Mword)this, current_cpu(), 0);

  switch (c->queue_item()->status())
    {
    case Queue_item::Ok:      return Not_locked;
    case Queue_item::Invalid: return Invalid;
    case Queue_item::Retry:   assert_kdb (false);
    }

  return Invalid;
}

PUBLIC inline NEEDS[Mp_lock::test_and_set]
Mp_lock::Status
Mp_lock::lock()
{ return test_and_set(); }

PUBLIC inline NEEDS["context.h", "cpu_lock.h", "globals.h",
                    "kdb_ke.h", "lock_guard.h"]
void
Mp_lock::clear()
{
  //assert_kdb (cpu_lock.test());

  Queue_item *f;

    {
      Lock_guard<Queue::Inner_lock> guard(_q.q_lock());
      //LOG_MSG_3VAL(current(), "clear", (Mword)this, current_cpu(), *((Mword*)this));
      assert_kdb (_q.blocked());

      f = _q.first();
      if (!f)
	{
	  _q.unblock();
	  return;
	}

      _q.dequeue(f, Queue_item::Ok);
    }
  // LOG_MSG_3VAL(current(), "wake", Mword(context_of(f)), (Mword)this, current_cpu());
  assert_kdb (current()->state() & Thread_ready_mask);
  context_of(f)->activate();
}

PUBLIC inline NEEDS["kdb_ke.h"]
void
Mp_lock::wait_free()
{

  Context *const c = current();

  Lock_guard<Cpu_lock> g(&cpu_lock);

    {
      Lock_guard<Queue::Inner_lock> guard(_q.q_lock());
      assert_kdb (invalid());

      if (!_q.blocked())
	return;

      _q.enqueue(c->queue_item());
    }

  while (1)
    {
      c->state_change_dirty(~Thread_ready, Thread_waiting);
      c->schedule();

      if (!c->queue_item()->queued())
	break;
    }

  c->state_del_dirty(Thread_waiting);
}

PUBLIC inline
bool
Mp_lock::test() const
{ return _q.blocked(); }


PUBLIC inline NEEDS[Mp_lock::clear]
void
Mp_lock::set(Status s)
{
  if (s == Not_locked)
    clear();
}

PUBLIC
void
Mp_lock::invalidate()
{
    {
      Lock_guard<Queue::Inner_lock> guard(_q.q_lock());
      _q.invalidate();
    }
      
  Queue_item *f;
  while (1)
    {
      Lock_guard<Queue::Inner_lock> guard(_q.q_lock());
      f = _q.first();

      //LOG_MSG_3VAL(current(), "deq", Mword(f), 0, 0);
      if (!f)
	break;

      _q.dequeue(f, Queue_item::Invalid);
      context_of(f)->activate();
    }
}

PUBLIC inline
bool
Mp_lock::invalid() const
{ return _q.invalid(); }
