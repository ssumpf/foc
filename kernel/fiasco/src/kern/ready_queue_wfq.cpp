INTERFACE[sched_wfq || sched_fp_wfq]:

#include "member_offs.h"
#include "types.h"
#include "globals.h"

template< typename E >
class Ready_queue_wfq
{
  friend class Jdb_thread_list;
  template<typename T>
  friend class Jdb_thread_list_policy;

public:
  E *current_sched() const { return _current_sched; }
  void activate(E *s) { _current_sched = s; }
  E *idle;

  void set_idle(E *sc)
  {
    idle = sc;
    E::wfq_elem(sc)->_ready_link = &idle;
    E::wfq_elem(sc)->_idle = 1;
  }

  void enqueue(E *);
  void dequeue(E *);
  E *next_to_run() const;

private:
  void swap(unsigned a, unsigned b);
  void heap_up(unsigned a);
  void heap_down(unsigned a);

  E *_current_sched;
  unsigned _cnt;
  E *_heap[1024];
};


// --------------------------------------------------------------------------
IMPLEMENTATION [sched_wfq || sched_fp_wfq]:

#include <cassert>
#include "config.h"
#include "cpu_lock.h"
#include "kdb_ke.h"
#include "std_macros.h"


IMPLEMENT inline
template<typename E>
E *
Ready_queue_wfq<E>::next_to_run() const
{
  if (_cnt)
    return _heap[0];

  if (_current_sched)
    E::wfq_elem(idle)->_dl = E::wfq_elem(_current_sched)->_dl;

  return idle;
}

IMPLEMENT inline
template<typename E>
void
Ready_queue_wfq<E>::swap(unsigned a, unsigned b)
{
  E::wfq_elem(_heap[a])->_ready_link = &_heap[b];
  E::wfq_elem(_heap[b])->_ready_link = &_heap[a];
  E *s = _heap[a];
  _heap[a] = _heap[b];
  _heap[b] = s;
}

IMPLEMENT inline
template<typename E>
void
Ready_queue_wfq<E>::heap_up(unsigned a)
{
  for (;a > 0;)
    {
      unsigned p = (a-1)/2;
      if (*E::wfq_elem(_heap[p]) < *E::wfq_elem(_heap[a]))
	return;
      swap(p, a);
      a = p;
    }
}

IMPLEMENT inline
template<typename E>
void
Ready_queue_wfq<E>::heap_down(unsigned a)
{
  for (;;)
    {
      unsigned c1 = 2*a + 1;
      unsigned c2 = 2*a + 2;

      if (_cnt <= c1)
	return;

      if (_cnt > c2 && *E::wfq_elem(_heap[c2]) <= *E::wfq_elem(_heap[c1]))
	c1 = c2;

      if (*E::wfq_elem(_heap[a]) <= *E::wfq_elem(_heap[c1]))
	return;

      swap(c1, a);

      a = c1;
    }
}

/**
 * Enqueue context in ready-list.
 */
IMPLEMENT
template<typename E>
void
Ready_queue_wfq<E>::enqueue(E *i)
{
  assert_kdb(cpu_lock.test());

  // Don't enqueue threads which are already enqueued
  if (EXPECT_FALSE (i->in_ready_list()))
    return;

  unsigned n = _cnt++;

  E *&h = _heap[n];
  h = i;
  E::wfq_elem(i)->_ready_link = &h;

  heap_up(n);
}

/**
 * Remove context from ready-list.
 */
IMPLEMENT inline NEEDS ["cpu_lock.h", "kdb_ke.h", "std_macros.h"]
template<typename E>
void
Ready_queue_wfq<E>::dequeue(E *i)
{
  assert_kdb (cpu_lock.test());

  // Don't dequeue threads which aren't enqueued
  if (EXPECT_FALSE (!i->in_ready_list() || i == idle))
    return;

  unsigned x = E::wfq_elem(i)->_ready_link - _heap;

  if (x == --_cnt)
    {
      E::wfq_elem(i)->_ready_link = 0;
      return;
    }

  swap(x, _cnt);
  heap_down(x);
  E::wfq_elem(i)->_ready_link = 0;
}

/**
 * Enqueue context in ready-list.
 */
PUBLIC
template<typename E>
void
Ready_queue_wfq<E>::requeue(E *i)
{
  if (!i->in_ready_list())
    enqueue(i);

  heap_down(E::wfq_elem(i)->_ready_link - _heap);
}

