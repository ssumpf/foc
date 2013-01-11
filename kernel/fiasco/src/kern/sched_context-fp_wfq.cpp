INTERFACE [sched_fp_wfq]:

#include "ready_queue_fp.h"
#include "ready_queue_wfq.h"

class Sched_context
{
  MEMBER_OFFSET();
  friend class Jdb_list_timeouts;
  friend class Jdb_thread_list;

  struct Ready_list_item_concept
  {
    typedef Sched_context Item;
    static Sched_context *&next(Sched_context *e) { return e->_sc.fp._ready_next; }
    static Sched_context *&prev(Sched_context *e) { return e->_sc.fp._ready_prev; }
    static Sched_context const *next(Sched_context const *e)
    { return e->_sc.fp._ready_next; }
    static Sched_context const *prev(Sched_context const *e)
    { return e->_sc.fp._ready_prev; }
  };

public:
  enum Type { Fixed_prio, Wfq };

  typedef cxx::Sd_list<Sched_context, Ready_list_item_concept> Fp_list;

private:
  Type _t;

  struct B_sc
  {
    unsigned short _p;
    unsigned _q;
    Unsigned64 _left;

    unsigned prio() const { return _p; }
  };


  struct Fp_sc : public B_sc
  {
    Sched_context *_ready_next, *_ready_prev;
  };

  struct Wfq_sc : public B_sc
  {
    Sched_context **_ready_link;
    bool _idle:1;
    Unsigned64 _dl;

    unsigned _w;
    unsigned _qdw;
    bool operator <= (Wfq_sc const &o) const
    { return _dl <= o._dl; }

    bool operator < (Wfq_sc const &o) const
    { return _dl < o._dl; }
  };

  union Sc
  {
    Wfq_sc wfq;
    Fp_sc fp;
  };

  Sc _sc;

public:
  static Wfq_sc *wfq_elem(Sched_context *x) { return &x->_sc.wfq; }

  struct Ready_queue
  {
  public:
    Ready_queue_fp<Sched_context> fp_rq;
    Ready_queue_wfq<Sched_context> wfq_rq;
    Context *schedule_in_progress;
    Sched_context *current_sched() const { return _current_sched; }
    void activate(Sched_context *s)
    {
      if (s && s->_t == Wfq)
	wfq_rq.activate(s);
      _current_sched = s;
    }

  private:
    Sched_context *_current_sched;

    friend class Jdb_thread_list;

  public:
    void set_idle(Sched_context *sc)
    { sc->_t = Wfq; sc->_sc.wfq._p = 0; wfq_rq.set_idle(sc); }

    void enqueue(Sched_context *);
    void dequeue(Sched_context *);
    Sched_context *next_to_run() const;
  };

  Context *context() const { return context_of(this); }
};


IMPLEMENTATION [sched_fp_wfq]:

#include <cassert>
#include "cpu_lock.h"
#include "kdb_ke.h"
#include "std_macros.h"
#include "config.h"

/**
 * Constructor
 */
PUBLIC
Sched_context::Sched_context()
{
  _t = Fixed_prio;
  _sc.fp._p = Config::Default_prio;
  _sc.fp._q = Config::Default_time_slice;
  _sc.fp._left = Config::Default_time_slice;
  _sc.fp._ready_next = 0;
}

IMPLEMENT inline
Sched_context *
Sched_context::Ready_queue::next_to_run() const
{
  Sched_context *s = fp_rq.next_to_run();
  if (s)
    return s;

  return wfq_rq.next_to_run();
}

/**
 * Check if Context is in ready-list.
 * @return 1 if thread is in ready-list, 0 otherwise
 */
PUBLIC inline
Mword
Sched_context::in_ready_list() const
{
  // this magically works for the fp list and the heap,
  // because wfq._ready_link and fp._ready_next are the
  // same memory location
  return _sc.wfq._ready_link != 0;
}

/**
 * Return if there is currently a schedule() in progress
 */
PUBLIC static inline
Context *
Sched_context::schedule_in_progress(unsigned cpu)
{
  return _ready_q.cpu(cpu).schedule_in_progress;
}

PUBLIC static inline
void
Sched_context::reset_schedule_in_progress(unsigned cpu)
{ _ready_q.cpu(cpu).schedule_in_progress = 0; }


PUBLIC inline
unsigned
Sched_context::prio() const
{ return _sc.fp._p; }

PUBLIC inline
void
Sched_context::set_prio(unsigned p)
{
  if (_t == Fixed_prio)
    _sc.fp._p = p;
  else
    _sc.fp._p = 0;
}

/**
 * Invalidate (expire) currently active global Sched_context.
 */
PUBLIC static inline
void
Sched_context::invalidate_sched(unsigned cpu)
{
  _ready_q.cpu(cpu).activate(0);
}

PUBLIC inline
void
Sched_context::deblock_refill(unsigned cpu)
{
  if (_t != Wfq)
    return;

  Unsigned64 da = 0;
  Sched_context *cs = _ready_q.cpu(cpu).wfq_rq.current_sched();

  if (EXPECT_TRUE(cs != 0))
    da = cs->_sc.wfq._dl;

  if (_sc.wfq._dl >= da)
    return;

  _sc.wfq._left += (da - _sc.wfq._dl) * _sc.wfq._w;
  if (_sc.wfq._left > _sc.wfq._q)
    _sc.wfq._left = _sc.wfq._q;
  _sc.wfq._dl = da;
}

/**
 * Enqueue context in ready-list.
 */
PUBLIC
void
Sched_context::ready_enqueue(unsigned cpu)
{
  assert_kdb(cpu_lock.test());

  // Don't enqueue threads which are already enqueued
  if (EXPECT_FALSE (in_ready_list()))
    return;

  Ready_queue &rq = _ready_q.cpu(cpu);

  if (_t == Fixed_prio)
    rq.fp_rq.enqueue(this, this == rq.current_sched());
  else
    rq.wfq_rq.enqueue(this);
}

/**
 * Remove context from ready-list.
 */
PUBLIC inline NEEDS ["cpu_lock.h", "kdb_ke.h", "std_macros.h"]
void
Sched_context::ready_dequeue()
{
  assert_kdb (cpu_lock.test());

  // Don't dequeue threads which aren't enqueued
  if (EXPECT_FALSE (!in_ready_list()))
    return;

  unsigned cpu = current_cpu();

  if (_t == Fixed_prio)
    _ready_q.cpu(cpu).fp_rq.dequeue(this);
  else
    _ready_q.cpu(cpu).wfq_rq.dequeue(this);
}

PUBLIC
void
Sched_context::requeue(unsigned cpu)
{
  if (_t == Fixed_prio)
    _ready_q.cpu(cpu).fp_rq.requeue(this);
  else
    _ready_q.cpu(cpu).wfq_rq.requeue(this);
}

PUBLIC inline
bool
Sched_context::dominates(Sched_context *sc)
{
  if (_t == Fixed_prio)
    return prio() > sc->prio();

  if (_sc.wfq._idle)
    return false;

  if (sc->_t == Fixed_prio)
    return false;

  return _sc.wfq._dl < sc->_sc.wfq._dl;
}

PUBLIC inline
void
Sched_context::replenish()
{
  _sc.fp._left = _sc.fp._q;
  if (_t == Wfq)
    _sc.wfq._dl += _sc.wfq._qdw;
}


PUBLIC inline
void
Sched_context::set_left(Unsigned64 l)
{ _sc.fp._left = l; }

PUBLIC inline
Unsigned64
Sched_context::left() const
{ return _sc.fp._left; }

PUBLIC inline
Unsigned64
Sched_context::quantum() const
{ return _sc.fp._q; }

PUBLIC inline
void
Sched_context::set_quantum(Unsigned64 q)
{ _sc.fp._q = q; }
