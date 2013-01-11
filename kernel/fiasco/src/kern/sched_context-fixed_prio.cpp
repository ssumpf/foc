
/*
 * Timeslice infrastructure
 */

INTERFACE [sched_fixed_prio]:

#include <dlist>
#include "member_offs.h"
#include "types.h"
#include "globals.h"
#include "ready_queue_fp.h"


class Sched_context : public cxx::D_list_item
{
  MEMBER_OFFSET();
  friend class Jdb_list_timeouts;
  friend class Jdb_thread_list;

  template<typename T>
  friend struct Jdb_thread_list_policy;

public:
  typedef cxx::Sd_list<Sched_context> Fp_list;

  class Ready_queue : public Ready_queue_fp<Sched_context>
  {
  public:
    Context *schedule_in_progress;
    void activate(Sched_context *s)
    { _current_sched = s; }
    Sched_context *current_sched() const { return _current_sched; }

  private:
    Sched_context *_current_sched;
  };

  Context *context() const { return context_of(this); }

private:
  unsigned short _prio;
  Unsigned64 _quantum;
  Unsigned64 _left;

  friend class Ready_queue_fp<Sched_context>;
};


IMPLEMENTATION [sched_fixed_prio]:

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
: _prio(Config::Default_prio),
  _quantum(Config::Default_time_slice),
  _left(Config::Default_time_slice)
{}


/**
 * Return priority of Sched_context
 */
PUBLIC inline
unsigned short
Sched_context::prio() const
{
  return _prio;
}

/**
 * Set priority of Sched_context
 */
PUBLIC inline
void
Sched_context::set_prio (unsigned short const prio)
{
  _prio = prio;
}

/**
 * Return full time quantum of Sched_context
 */
PUBLIC inline
Unsigned64
Sched_context::quantum() const
{
  return _quantum;
}

/**
 * Set full time quantum of Sched_context
 */
PUBLIC inline
void
Sched_context::set_quantum (Unsigned64 const quantum)
{
  _quantum = quantum;
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
{ set_left(quantum()); }

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
  return Fp_list::in_list(this);
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
PUBLIC inline
void
Sched_context::ready_enqueue(unsigned cpu)
{
  assert_kdb(cpu_lock.test());

  // Don't enqueue threads which are already enqueued
  if (EXPECT_FALSE (in_ready_list()))
    return;

  Ready_queue &rq = _ready_q.cpu(cpu);

  rq.enqueue(this, this == rq.current_sched());
}

PUBLIC inline
void
Sched_context::requeue(unsigned cpu)
{
  _ready_q.cpu(cpu).requeue(this);
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

PUBLIC inline
bool
Sched_context::dominates(Sched_context *sc)
{ return prio() > sc->prio(); }

PUBLIC inline
void
Sched_context::deblock_refill(unsigned)
{}

