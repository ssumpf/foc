INTERFACE:
#include "per_cpu_data.h"

EXTENSION class Sched_context
{
private:
  static Per_cpu<Ready_queue> _ready_q;
};

IMPLEMENTATION:

#include "kdb_ke.h"

DEFINE_PER_CPU Per_cpu<Sched_context::Ready_queue> Sched_context::_ready_q;

PUBLIC static inline
Sched_context::Ready_queue &
Sched_context::rq(unsigned cpu)
{ return _ready_q.cpu(cpu); }


/**
 * \param cpu must be current_cpu()
 */
PUBLIC inline NEEDS["kdb_ke.h"]
void
Sched_context::deblock(unsigned cpu)
{
  assert_kdb(cpu_lock.test());

  Sched_context *cs = rq(cpu).current_sched();
  if (this != cs)
      deblock_refill(cpu);

  ready_enqueue(cpu);
}

/**
 * \param cpu must be current_cpu()
 * \param crs the Sched_context of the current context
 * \param lazy_q queue lazily if applicable
 */
PUBLIC inline NEEDS["kdb_ke.h"]
bool
Sched_context::deblock(unsigned cpu, Sched_context *crs, bool lazy_q = false)
{
  assert_kdb(cpu_lock.test());

  Sched_context *cs = rq(cpu).current_sched();
  bool res = true;
  if (this == cs)
    {
      if (crs->dominates(this))
	res = false;
    }
  else
    {
      deblock_refill(cpu);

      if ((EXPECT_TRUE(cs != 0) && cs->dominates(this)) || crs->dominates(this))
	res = false;
    }

  if (res && lazy_q)
    return true;

  ready_enqueue(cpu);
  return res;
}

