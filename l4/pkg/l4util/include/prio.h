/*!
 * \file
 * \brief  Thread priority related functions.
 *
 * \date   11/22/2005
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/*
 * (c) 2004-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */
#ifndef __L4UTIL__PRIO_H__
#define __L4UTIL__PRIO_H__

#include <l4/sys/syscalls.h>

EXTERN_C_BEGIN

/**
 * \defgroup l4util_prio Priority related functions
 * \ingroup l4util_api
 */
/*@{*/

/**
 * \brief Set priority of a thread.
 *
 * \param id	Id of the thread to set the priority for
 * \param prio	Priority of the thread.
 *
 * \return 0 for success, !0 for failure.
 */
static inline int
l4util_prio_thread_set(l4_threadid_t id, unsigned prio)
{
  l4_sched_param_t p;
  l4_threadid_t dummy = L4_INVALID_ID;

  l4_thread_schedule(id, L4_INVALID_SCHED_PARAM, &dummy, &dummy, &p);

  if (!l4_is_invalid_sched_param(p))
    {
      p.sp.prio  = prio;
      p.sp.state = 0;
      l4_thread_schedule(id, p, &dummy, &dummy, &p);
      if (!l4_is_invalid_sched_param(p))
	return 0;
    }

  return 1;
}
/*@}*/
EXTERN_C_END

#endif /* ! __L4UTIL__PRIO_H__ */
