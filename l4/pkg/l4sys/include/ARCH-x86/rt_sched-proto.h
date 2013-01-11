/**
 * \file
 * \brief  Identifier and prototype definitions for real-time scheduling
 */
/*
 * (c) 2005-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Björn Döbel <doebel@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *               Jork Löser <jork@os.inf.tu-dresden.de>
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
#ifndef __SYS_INCLUDE_ARCH_X86_RT_SCHED_PROTO_H_
#define __SYS_INCLUDE_ARCH_X86_RT_SCHED_PROTO_H_

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>

#define L4_RT_ADD_TIMESLICE	1   /**< Add time slice */
#define L4_RT_REM_TIMESLICES	2   /**< Remove time slice */
#define L4_RT_SET_PERIOD	3   /**< Set period */
#define L4_RT_BEGIN_PERIODIC	4   /**< Begin periodic */
#define L4_RT_BEGIN_PERIODIC_NS	5   /**< Begin periodic */
#define L4_RT_END_PERIODIC	6   /**< End periodic */
#define L4_RT_CHANGE_TIMESLICE	7   /**< Change time slice */

#define L4_RT_NEXT_PERIOD	0x10 /**< Next period */
#define L4_RT_PREEMPTION_ID	0x20 /**< Preemption ID */

/** \brief Struct \internal */
typedef struct {
    l4_uint64_t time:56;        /**< time */
    l4_uint64_t id:7;           /**< id */
    l4_uint64_t type:1;         /**< id */
} l4_rt_preemption_val_t;
/** \brief Struct \internal */
typedef struct {
    l4_uint32_t time_high:24;   /**< time_high */
    l4_uint32_t id:7;           /**< id */
    l4_uint32_t type:1;         /**< type */
} l4_rt_preemption_val32_t;
/** \brief Struct \internal */
typedef union {
    l4_low_high_t lh;          /**< lh value */
    l4_rt_preemption_val_t p;  /**< p value */
} l4_rt_preemption_t;

 /* preemption ipc types: values of the type field */
#define L4_RT_PREEMPT_DEADLINE	0	/**< deadline miss */
#define L4_RT_PREEMPT_TIMESLICE	1	/**< time slice overrun */

/*!\brief Add a time slice for periodic execution.
 * \ingroup api_calls_rt_sched
 *
 * \param  dest		thread to add the time slice to
 * \param  prio		priority of the time slice
 * \param  time		length of the time slice in microseconds
 * \retval 0		OK
 * \retval -1		Error, one of:
 *			- dest does not exist
 *			- insufficient MCP (old or new prio>MCP),
 *			- dest running in periodic mode or transitioning to
 *			- time quantum 0 or infinite
 */
L4_INLINE int
l4_rt_add_time slice(l4_threadid_t dest, int prio, int time);

/*!\brief Change a time slice for periodic execution.
 * \ingroup api_calls_rt_sched
 *
 * \param  dest		thread whose timing parameters are to change
 * \param  id		number of the time-slice to change (rt start at 1)
 * \param  prio		new priority of the time slice
 * \param  time		new length of the time slice in microseconds,
 *			0: don't change.
 * \retval 0		OK
 * \retval -1		Error, one of:
 *			- dest does not exist
 *			- insufficient MCP (old or new prio>MCP),
 *			- time slice does not exist
 *
 * This function modifies the priority and optionally the length of an
 * existing time slice of a thread. When calling this function while
 * the time slice is active, the effect may be delayed till the next
 * period.
 *
 * This function can be called as soon as the denoted time slice was
 * added with l4_rt_add_time slice(). Thus, the thread may have started
 * periodic execution already, but it needs not.
 */
L4_INLINE int
l4_rt_change_time slice(l4_threadid_t dest, int id, int prio, int time);

/*!\brief Start strictly periodic execution
 * \ingroup api_calls_rt_sched
 *
 * \param  dest		thread that starts periodic execution
 * \param  clock	absolute time to start.
 * \retval 0		OK
 * \retval -1		Error, one of:
 *			- dest does not exist
 *			- insufficient MCP (old or new prio>MCP),
 *			- dest running in periodic mode or transitioning to
 *
 * Call this function to start the periodic execution after setting up
 * the time slices using l4_rt_add_time slice() and l4_rt_set_period(). 
 *
 * By the time specified in clock thread dest must wait for the next period,
 * e.g. by using l4_rt_next_period() or some other IPC with the
 * L4_RT_NEXT_PERIOD flag enabled. Otherwise the transition to periodic mode
 * fails.
 */
L4_INLINE int
l4_rt_begin_strictly_periodic(l4_threadid_t dest, l4_kernel_clock_t clock);

/*!\brief Start periodic execution with minimal inter-release times
 * \ingroup api_calls_rt_sched
 *
 * \param  dest		thread that starts periodic execution
 * \param  clock	absolute time to start.
 * \retval 0		OK
 * \retval -1		Error, one of:
 *			- dest does not exist
 *			- insufficient MCP (old or new prio>MCP),
 *			- dest running in periodic mode or transitioning to
 *
 * Call this function to start the periodic execution after setting up
 * the time slices using l4_rt_add_time slice() and l4_rt_set_period().
 *
 * By the time specified in clock thread dest must wait for the next period,
 * e.g. by using l4_rt_next_period() or some other IPC with the
 * L4_RT_NEXT_PERIOD flag enabled. Otherwise the transition to periodic mode
 * fails.
 */
L4_INLINE int
l4_rt_begin_minimal_periodic(l4_threadid_t dest, l4_kernel_clock_t clock);

/*!\brief Stop periodic execution
 * \ingroup api_calls_rt_sched
 *
 * \param  dest		thread that stops periodic execution
 * \retval 0		OK
 * \retval -1		Error, one of:
 *			- dest does not exist
 *			- insufficient MCP (old or new prio>MCP),
 *			- dest not running in periodic mode and
 *			  not transitioning to
 *
 * This function aborts the periodic execution of thread dest. Thread dest
 * returns to conventional scheduling then.
 */
L4_INLINE int
l4_rt_end_periodic(l4_threadid_t dest);

/*!\brief   Remove all reservation scheduling contexts
 * \ingroup api_calls_rt_sched
 *
 * This function removes all the scheduling contexts that were set up
 * so far for the given thread.
 *
 * \param  dest		thread the scheduling contexts should be removed from
 * \retval 0		OK
 * \retval -1		Error, one of:
 *			- dest does not exist
 *			- insufficient MCP
 *			- dest running in periodic mode or transitioning to
 */
L4_INLINE int
l4_rt_remove(l4_threadid_t dest);

/*!\brief Set the length of the period
 * \ingroup api_calls_rt_sched
 *
 * This function sets the length of the period for periodic execution.
 *
 * \param  dest		destination thread
 * \param  clock	period length in microseconds. Will be rounded up
 *			by the kernel according to the timer granularity.
 * \return		This function always succeeds.
 */
L4_INLINE void
l4_rt_set_period(l4_threadid_t dest, l4_kernel_clock_t clock);

/*!\brief activate the next time slice (scheduling context)
 * \ingroup api_calls_rt_sched
 *
 * \param  id		The ID of the time slice we think we are on
 *			(current time slice)
 * \param  clock	pointer to a l4_kernel_clock_t variable
 * \retval 0		OK, *clock contains the remaining time of the time slice
 * \retval -1		Error, id did not match current time slice
 *
 */
L4_INLINE int
l4_rt_next_reservation(unsigned id, l4_kernel_clock_t*clock);

/*!\brief Wait for the next period, skipping all unused time slices
 * \ingroup api_calls_rt_sched
 *
 * \retval 0		OK
 * \retval !0		IPC Error.
 *
 */
L4_INLINE int
l4_rt_next_period(void);

/*!\brief Return the preemption id of a thread
 * \ingroup api_calls_rt_sched
 *
 * \param  id		thread
 *
 * \return thread-id of the (virtual) preemption IPC sender
 */
L4_INLINE l4_threadid_t
l4_preemption_id(l4_threadid_t id);

/*!\brief Return thread-id that flags waiting for the next period
 * \ingroup api_calls_rt_sched
 *
 * \param  id		original thread-id
 *
 * \return modified id, to be used in an IPC, waiting for the next period.
 */
L4_INLINE l4_threadid_t
l4_next_period_id(l4_threadid_t id);

/*!\brief Generic real-time setup function
 * \ingroup api_calls_rt_sched
 *
 * \param  dest		destination thread
 * \param  param	scheduling parameter
 * \param  clock	clock parameter
 * \retval 0		OK
 * \retval -1		Error.
 *
 * This function is not meant to be used directly, it is merely used
 * by others.
 */
L4_INLINE int
l4_rt_generic(l4_threadid_t dest, l4_sched_param_t param,
		    l4_kernel_clock_t clock);

/*!\brief Delayed preemption: Reserve a duration
 *
 * \param  duration	time (microseconds)
 * \retval 0		OK
 * \retval !0		Error.
 *
 * This function reserves a non-preemptible duration at the kernel.
 *
 * \note Due to the currently missing support in the kernel, this
 *       call is a no-op.
 */
L4_INLINE int
l4_rt_dp_reserve(int duration);

/*!\brief Delayed preemption: Cancel a reservation
 *
 * \retval 0		OK
 * \retval !0		Error.
 */
L4_INLINE void
l4_rt_dp_remove(void);

/*!\brief Delayed preemption: Start a delayed preemption
 *
 * After making a dp reservation, a thread can start an uninterruptible
 * execution for the reserved time.
 *
 * \note Due to the currently missing support in the kernel, this
 *       call is implemented as a cli(), requiring cooperation from the
 *	 calling thread.
 */
L4_INLINE void
l4_rt_dp_begin(void);

/*!\brief Delayed preemption: End a delayed preemption
 *
 * After calling this function, the thread may be preempted by the kernel
 * again.
 */
L4_INLINE void
l4_rt_dp_end(void);

/***************************************************************************
 *** IMPLEMENTATIONS
 ***************************************************************************/

L4_INLINE l4_threadid_t
l4_preemption_id(l4_threadid_t id)
{
  //id.id.chief |= L4_RT_PREEMPTION_ID;
  return id;
}

L4_INLINE l4_threadid_t
l4_next_period_id(l4_threadid_t id)
{
  //id.id.chief |= L4_RT_NEXT_PERIOD;
  return id;
}

#endif /* ! __L4_SYS__RT_SCHED_H__ */
