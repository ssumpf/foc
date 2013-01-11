
/*
 * Timeslice infrastructure
 */

INTERFACE[sched_wfq] :

#include "member_offs.h"
#include "types.h"
#include "globals.h"
#include "ready_queue_wfq.h"


class Sched_context
{
  MEMBER_OFFSET();
  friend class Jdb_list_timeouts;
  friend class Jdb_thread_list;

  template<typename T>
  friend class Jdb_thread_list_policy;

public:
  class Ready_queue : public Ready_queue_wfq<Sched_context>
  {
  public:
    Context *schedule_in_progress;
  };

  Context *context() const { return context_of(this); }

private:
  static Sched_context *wfq_elem(Sched_context *x) { return x; }

  Sched_context **_ready_link;
  bool _idle:1;
  Unsigned64 _dl;
  Unsigned64 _left;

  unsigned _q;
  unsigned _w;
  unsigned _qdw;

  friend class Ready_queue_wfq<Sched_context>;

  bool operator <= (Sched_context const &o) const
  { return _dl <= o._dl; }

  bool operator < (Sched_context const &o) const
  { return _dl < o._dl; }

};

// --------------------------------------------------------------------------
IMPLEMENTATION [sched_wfq]:

#include <cassert>
#include "config.h"
#include "cpu_lock.h"
#include "kdb_ke.h"
#include "std_macros.h"


/**
 * Constructor
 */
PUBLIC
Sched_context::Sched_context()
: _ready_link(0),
  _idle(0),
  _dl(0),
  _left(Config::Default_time_slice),
  _q(Config::Default_time_slice),
  _w(1),
  _qdw(_q / _w)
{}

/**
 * Return owner of Sched_context
 */
PUBLIC inline
Context *
Sched_context::owner() const
{
  return context();
}

/**
 * Return full time quantum of Sched_context
 */
PUBLIC inline
Unsigned64
Sched_context::quantum() const
{
  return _q;
}

/**
 * Set full time quantum of Sched_context
 */
PUBLIC inline
void
Sched_context::set_quantum(Unsigned64 const quantum)
{
  _q = quantum;
  _qdw = _q/_w;
}

/**
 * Return remaining time quantum of Sched_context
 */
PUBLIC inline
Unsigned64
Sched_context::left() const
{
  return _left;
}

PUBLIC inline NEEDS[Sched_context::set_left, Sched_context::quantum]
void
Sched_context::replenish()
{
  set_left(_q);
  _dl += _qdw;
}

/**
 * Set remaining time quantum of Sched_context
 */
PUBLIC inline
void
Sched_context::set_left (Unsigned64 const left)
{
  _left = left;
}

/**
 * Check if Context is in ready-list.
 * @return 1 if thread is in ready-list, 0 otherwise
 */
PUBLIC inline
Mword
Sched_context::in_ready_list() const
{
  return _ready_link != 0;
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

  _ready_q.cpu(cpu).dequeue(this);

}

/**
 * Enqueue context in ready-list.
 */
PUBLIC inline NEEDS["kdb_ke.h"]
void
Sched_context::ready_enqueue(unsigned cpu)
{
  assert_kdb(cpu_lock.test());

  // Don't enqueue threads which are already enqueued
  if (EXPECT_FALSE (in_ready_list()))
    return;

  _ready_q.cpu(cpu).enqueue(this);;
}

PUBLIC
void
Sched_context::requeue(unsigned cpu)
{
  _ready_q.cpu(cpu).requeue(this);
}

PUBLIC inline
bool
Sched_context::dominates(Sched_context *sc)
{
#if 0
  if (_idle)
    LOG_MSG_3VAL(current(), "idl", (Mword)sc, _dl, sc->_dl);
#endif
  return !_idle && _dl < sc->_dl;
}

PUBLIC inline
void
Sched_context::deblock_refill(unsigned cpu)
{
  Unsigned64 da = 0;
  Sched_context *cs = rq(cpu).current_sched();
  if (EXPECT_TRUE(cs != 0))
    da = cs->_dl;

  if (_dl >= da)
    return;

  _left += (da - _dl) * _w;
  if (_left > _q)
    _left = _q;
  _dl = da;
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


/**
 * Invalidate (expire) currently active global Sched_context.
 */
PUBLIC static inline
void
Sched_context::invalidate_sched(unsigned cpu)
{
  _ready_q.cpu(cpu).activate(0);
}


PUBLIC static inline
unsigned
Sched_context::prio() { return 0; }

PUBLIC static inline
void
Sched_context::set_prio(unsigned)
{}
