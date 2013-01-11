/**
 * \file
 * \brief  Implementation of real-time scheduling system calls
 */
/*
 * (c) 2005-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
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
#ifndef __SYS_INCLUDE_ARCH_X86_RT_SCHED_IMPL_H_
#define __SYS_INCLUDE_ARCH_X86_RT_SCHED_IMPL_H_

#include <l4/sys/ipc.h>

L4_INLINE int
l4_rt_generic(l4_threadid_t dest, l4_sched_param_t param,
		    l4_kernel_clock_t clock)
{
  unsigned dummy;
  int ret;

  __asm__ __volatile__(
	  "pushl %%ebp		\n\t"	/* save ebp, no memory references
					   ("m") after this point */
	  L4_SYSCALL(thread_schedule)
	  "popl	 %%ebp		\n\t"	/* restore ebp, no memory references
					   ("m") before this point */
	 :
	  "=a" (ret),
	  "=c" (dummy),
	  "=d" (dummy),
	  "=S" (dummy)
	 :
	  "a" (param),
	  "c" ((l4_uint32_t)clock),
	  "d" ((l4_uint32_t)(clock>>32)),
	  "S" (dest.raw)
	 :
	  "ebx", "edi"
	 );
  return ret;
}


/* Erm, the user shall be allowed to pass "normal" time parameters, not
 * the mantissa/exponent coding. Thus, make a reference to the external
 * l4util function. */
L4_INLINE int
l4_rt_add_timeslice(l4_threadid_t dest, int prio, int time)
{
  l4_sched_param_t sched = {sp:{prio:prio, small:0,
				state:L4_RT_ADD_TIMESLICE}};
  l4_sched_param_set_time(time, &sched);
  return l4_rt_generic(dest, sched, 0);
}

L4_INLINE int
l4_rt_change_timeslice(l4_threadid_t dest, int id, int prio, int time)
{
  l4_sched_param_t sched = {sp:{prio:prio, small:id,
				state:L4_RT_CHANGE_TIMESLICE}};
  l4_sched_param_set_time(time, &sched);
  return l4_rt_generic(dest, sched, 0);
}

L4_INLINE int
l4_rt_begin_strictly_periodic (l4_threadid_t dest, l4_kernel_clock_t clock)
{
  return l4_rt_generic(dest,
			     (l4_sched_param_t){
			       sp:{ prio:0, small:0,
				    state:L4_RT_BEGIN_PERIODIC}},
			     clock);
}

L4_INLINE int
l4_rt_begin_minimal_periodic (l4_threadid_t dest, l4_kernel_clock_t clock)
{
  return l4_rt_generic(dest,
			     (l4_sched_param_t){
			       sp:{prio:0, small: 0,
				   state:L4_RT_BEGIN_PERIODIC_NS}},
			     clock);
}

L4_INLINE int
l4_rt_end_periodic(l4_threadid_t dest)
{
  return l4_rt_generic(dest,
		       (l4_sched_param_t){
			 sp:{prio:0, small:0,
			     state:L4_RT_END_PERIODIC}}, 0);
}

L4_INLINE int
l4_rt_remove(l4_threadid_t dest)
{
  return l4_rt_generic(dest,
		       (l4_sched_param_t){
			 sp:{prio:0, small:0,
			     state:L4_RT_REM_TIMESLICES}}, 0);
}

L4_INLINE void
l4_rt_set_period(l4_threadid_t dest, l4_kernel_clock_t clock)
{
  l4_rt_generic(dest,
		      (l4_sched_param_t){
			sp:{prio:0, small:0,
			    state:L4_RT_SET_PERIOD}},
		      clock);
}

L4_INLINE int
l4_rt_next_reservation(unsigned id, l4_kernel_clock_t*clock)
{
  long dummy;
  int ret;
  
  __asm__ __volatile__(
          "pushl %%ebp          \n\t"   /* save ebp, no memory references
                                           ("m") after this point */     
          L4_SYSCALL(thread_switch)
          "popl  %%ebp          \n\t"   /* restore ebp, no memory references
                                           ("m") before this point */
          :
          "=a" (ret),
	  "=c" (((l4_low_high_t*)clock)->low),
	  "=d" (((l4_low_high_t*)clock)->high),
          "=S" (dummy)
          :
          "S" (0),
          "a" (id)
          :
          "ebx", "edi"
          );
  return ret;
}

L4_INLINE int
l4_rt_next_period(void)
{
  l4_umword_t dummy;
  l4_msgdope_t result;
  return l4_ipc_receive(l4_next_period_id(L4_NIL_ID),
			L4_IPC_SHORT_MSG,
			&dummy, &dummy, L4_IPC_BOTH_TIMEOUT_0,
			&result);
}

L4_INLINE int
l4_rt_dp_reserve(int duration)
{
  /* no-op */
  return 0;
}

L4_INLINE void
l4_rt_dp_remove(void){
  /* no-op */
}

L4_INLINE void
l4_rt_dp_begin(void){
  __asm__ __volatile__ ("cli" : : : "memory");
}

L4_INLINE void
l4_rt_dp_end(void){
  __asm__ __volatile__ ("sti" : : : "memory");
}
#endif /* ! __L4_SYS__RT_SCHED_H__ */
