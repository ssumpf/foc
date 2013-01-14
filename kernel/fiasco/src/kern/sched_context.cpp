INTERFACE:
#include "per_cpu_data.h"

EXTENSION class Sched_context
{
public:
  class Ready_queue : public Ready_queue_base
  {
  public:
    void set_current_sched(Sched_context *sched);
    void invalidate_sched() { activate(0); }
    bool deblock(Sched_context *sc, Sched_context *crs, bool lazy_q = false);
    void deblock(Sched_context *sc);
    void ready_enqueue(Sched_context *sc)
    {
      assert_kdb(cpu_lock.test());

      // Don't enqueue threads which are already enqueued
      if (EXPECT_FALSE (sc->in_ready_list()))
        return;

      enqueue(sc, sc == current_sched());
    }

    void ready_dequeue(Sched_context *sc)
    {
      assert_kdb (cpu_lock.test());

      // Don't dequeue threads which aren't enqueued
      if (EXPECT_FALSE (!sc->in_ready_list()))
        return;

      dequeue(sc);
    }

    void switch_sched(Sched_context *from, Sched_context *to)
    {
      assert_kdb (cpu_lock.test());

      // If we're leaving the global timeslice, invalidate it This causes
      // schedule() to select a new timeslice via set_current_sched()
      if (from == current_sched())
        invalidate_sched();

      if (from->in_ready_list())
        dequeue(from);

      enqueue(to, false);
    }

    Context *schedule_in_progress;
  };

  static Per_cpu<Ready_queue> rq;
};

IMPLEMENTATION:

#include "kdb_ke.h"
#include "timer.h"
#include "timeout.h"
#include "globals.h"
#include "logdefs.h"

DEFINE_PER_CPU Per_cpu<Sched_context::Ready_queue> Sched_context::rq;

/**
 * Set currently active global Sched_context.
 */
IMPLEMENT
void
Sched_context::Ready_queue::set_current_sched(Sched_context *sched)
{
  assert_kdb (sched);
  // Save remainder of previous timeslice or refresh it, unless it had
  // been invalidated
  Timeout * const tt = timeslice_timeout.current();
  Unsigned64 clock = Timer::system_clock();
  if (Sched_context *s = current_sched())
    {
      Signed64 left = tt->get_timeout(clock);
      if (left > 0)
	s->set_left(left);
      else
	s->replenish();

      LOG_SCHED_SAVE(s);
    }

  // Program new end-of-timeslice timeout
  tt->reset();
  tt->set(clock + sched->left(), current_cpu());

  // Make this timeslice current
  activate(sched);

  LOG_SCHED_LOAD(sched);
}


/**
 * \param cpu must be current_cpu()
 */
IMPLEMENT inline NEEDS["kdb_ke.h"]
void
Sched_context::Ready_queue::deblock(Sched_context *sc)
{
  assert_kdb(cpu_lock.test());

  Sched_context *cs = current_sched();
  if (sc != cs)
      deblock_refill(sc);

  ready_enqueue(sc);
}

/**
 * \param cpu must be current_cpu()
 * \param crs the Sched_context of the current context
 * \param lazy_q queue lazily if applicable
 */
IMPLEMENT inline NEEDS["kdb_ke.h"]
bool
Sched_context::Ready_queue::deblock(Sched_context *sc, Sched_context *crs, bool lazy_q)
{
  assert_kdb(cpu_lock.test());

  Sched_context *cs = current_sched();
  bool res = true;
  if (sc == cs)
    {
      if (crs->dominates(sc))
	res = false;
    }
  else
    {
      deblock_refill(sc);

      if ((EXPECT_TRUE(cs != 0) && cs->dominates(sc)) || crs->dominates(sc))
	res = false;
    }

  if (res && lazy_q)
    return true;

  ready_enqueue(sc);
  return res;
}

