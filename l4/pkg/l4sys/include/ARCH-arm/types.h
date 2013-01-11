/**
 * \file
 * \brief Types for ARM
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 */
#ifndef L4_TYPES_H
#define L4_TYPES_H

#include_next <l4/sys/types.h>

#include <l4/sys/l4int.h>
#include <l4/sys/compiler.h>
#include <l4/sys/consts.h>

#include <l4/sys/__l4_fpage.h>

#include <l4/sys/__timeout.h>

/**
 * \brief l4_schedule param word
 */
/*
typedef union {
  struct {
    l4_umword_t prio     :8;
    l4_umword_t small    :8;
    l4_umword_t state    :4;
    l4_umword_t time_exp :4;
    l4_umword_t time_man :8;
  } sp;
  l4_umword_t sched_param;
  l4_umword_t raw;
} l4_sched_param_t;

#define L4_INVALID_SCHED_PARAM ((l4_sched_param_t){raw:(l4_umword_t)-1})
*/

/* Compute l4_sched_param_struct_t->small argument for
   l4_thread_schedule(): size_mb is the size of all small address
   spaces, and nr is the number of the small address space.  See
   Liedtke: ``L4 Pentium implementation'' */
//#define L4_SMALL_SPACE(size_mb, nr) ((size_mb >> 2) + nr * (size_mb >> 1))

/*
 * Some useful operations and test functions for id's and types
 */

//L4_INLINE int        l4_is_invalid_sched_param (l4_sched_param_t sp);

/*-----------------------------------------------------------------------------
 * IMPLEMENTATION
 *---------------------------------------------------------------------------*/
/*
L4_INLINE int l4_is_invalid_sched_param(l4_sched_param_t sp)
{
  return sp.raw == (l4_umword_t)-1;
}
*/


#endif /* L4_TYPES_H */ 


