/*****************************************************************************/
/**
 * \file
 * \brief   Better lock implementation (in comparison to lock.h). In the
 *          case of contention we are going into sleep and wait for the
 *          current locker to be woken up. Nevertheless this code has
 *          some limitations considering different thread priorities.
 *
 * \author  Jork Loeser <hohmuth@os.inf.tu-dresden.de> */

/*
 * (c) 2003-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */

#ifndef __L4UTIL_LOCK_WQ_H__
#define __L4UTIL_LOCK_WQ_H__

#include <l4/sys/types.h>
//#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/util/atomic.h>

EXTERN_C_BEGIN

typedef struct l4util_wq_lock_queue_elem_t
{
  volatile struct l4util_wq_lock_queue_elem_t *next, *prev;
  l4_cap_idx_t    id;
} l4util_wq_lock_queue_elem_t;

typedef struct
{
  l4util_wq_lock_queue_elem_t *last;
} l4util_wq_lock_queue_base_t;

static inline int
l4util_wq_lock_lock(l4util_wq_lock_queue_base_t *queue,
		    l4util_wq_lock_queue_elem_t *q,
                    l4_cap_idx_t myself);
static inline int
l4util_wq_lock_unlock(l4util_wq_lock_queue_base_t *queue,
		      l4util_wq_lock_queue_elem_t *q);
static inline int
l4util_wq_lock_locked(l4util_wq_lock_queue_base_t *queue);

/* Implementation */
inline int
l4util_wq_lock_lock(l4util_wq_lock_queue_base_t *queue,
		    l4util_wq_lock_queue_elem_t *q,
                    l4_cap_idx_t myself)
{
  l4util_wq_lock_queue_elem_t *old;
  l4_msgtag_t err;

  q->next = 0;
  q->id   = myself;
  old     = (l4util_wq_lock_queue_elem_t*)
		    l4util_xchg((l4_umword_t*)(&queue->last), (l4_umword_t)q);
  if (old != 0)
    {
      /* already locked */
      old->next = q;
      q->prev   = old;
      err = l4_ipc_receive(old->id, L4_IPC_NEVER);
      if (L4_IPC_ERROR(l4_utcb()->error))
	return l4_utcb()->error;
      err = l4_ipc_send(old->id, l4_msgtag(0, 0, 0, 0), L4_IPC_NEVER);
      if (L4_IPC_ERROR(l4_utcb()->error))
	return l4_utcb()->error;
    }
  return 0;
}

inline int
l4util_wq_lock_unlock(l4util_wq_lock_queue_base_t *queue,
                      l4util_wq_lock_queue_elem_t *q)
{
  volatile l4util_wq_lock_queue_elem_t *other;
  l4_msgtag_t err;

  other = (l4util_wq_lock_queue_elem_t*)
		  l4util_cmpxchg_res((l4_umword_t*)(&queue->last),
                                                    (l4_umword_t)q,
                                                    (l4_umword_t)NULL);
  if (other == q)
    {
      /* nobody wants the lock */
    }
  else
    {
      /* someone wants the lock */
      while(q->next != other)
	{
	  /* 2 possibilities:
	     - other is next, but didn't sign, give it the time
	     - other is not next, find the next by backward iteration */
	  if (other->prev == NULL)
	    {
	      /* - other didn't sign its prev, give it the time to do this */
	      //l4_thread_switch(other->id);
              // FIXME: what to do?
	    }
	  else if (other->prev!=q)
	    {
	      /* 2 poss:
		 - if other is next it might be signed up to now
		   (other->prev == q)
		   - other is not something else then next (its not NULL,
		     we know this), go backward */
	      other = other->prev;
	    }
	}
      /* now we have the next in other */
      /* send an ipc, timeout never */
      err = l4_ipc_call(q->next->id, l4_msgtag(0, 0, 0, 0), L4_IPC_NEVER);
      if (L4_IPC_IS_ERROR(l4_utcb()->error))
	return L4_IPC_ERROR(l4_utcb()->error);
    }
  return 0;
}

inline int
l4util_wq_lock_locked(l4util_wq_lock_queue_base_t *queue)
{
  return queue->last != NULL;
}

EXTERN_C_END

#endif
