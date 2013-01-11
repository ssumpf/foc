/**
 * \file
 * \brief  IPC system calls for X86
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
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
#pragma once

#include <l4/sys/consts.h>

L4_INLINE l4_msgtag_t
l4_ipc_call(l4_cap_idx_t dest, l4_utcb_t *u,
            l4_msgtag_t snd_tag,
            l4_timeout_t timeout) L4_NOTHROW
{
  l4_umword_t dummy1, dummy2, dummy3;
  l4_msgtag_t rtag;

  (void)u;

  __asm__ __volatile__
    (L4_ENTER_KERNEL
     :
     "=d" (dummy3),
     "=D" (dummy1),
     "=c" (timeout),
     "=S" (dummy2),
     "=a" (rtag.raw)
     :
     "S" (0),
     "c" (timeout),
     "d" (dest | L4_SYSF_CALL),
     "a" (snd_tag.raw)
     :
     "memory", "cc" L4S_PIC_CLOBBER
     );
  return rtag;
}


L4_INLINE l4_msgtag_t
l4_ipc_reply_and_wait(l4_utcb_t *u, l4_msgtag_t tag,
                      l4_umword_t *label,
                      l4_timeout_t timeout) L4_NOTHROW
{
  l4_umword_t dummy, dummy1, dummy2;
  l4_msgtag_t rtag;

  (void)u;

  __asm__ __volatile__
    (L4_ENTER_KERNEL
     :
     "=d" (dummy2),
     "=S" (*label),
     "=c" (dummy1),
     "=D" (dummy),
     "=a" (rtag.raw)
     :
     "S" (0),
     "c" (timeout),
     "a" (tag.raw),
     "d" (L4_INVALID_CAP | L4_SYSF_REPLY_AND_WAIT)
     :
     "memory", "cc" L4S_PIC_CLOBBER
     );
  return rtag;
}


L4_INLINE l4_msgtag_t
l4_ipc_send_and_wait(l4_cap_idx_t dest, l4_utcb_t *u,
                     l4_msgtag_t tag,
                     l4_umword_t *label,
                     l4_timeout_t timeout) L4_NOTHROW
{
  l4_umword_t dummy, dummy1, dummy2;
  l4_msgtag_t rtag;

  (void)u;

  __asm__ __volatile__
    (L4_ENTER_KERNEL
     :
     "=d" (dummy2),
     "=S" (*label),
     "=c" (dummy1),
     "=D" (dummy),
     "=a" (rtag.raw)
     :
     "S" (0),
     "c" (timeout),
     "a" (tag.raw),
     "d" (dest | L4_SYSF_SEND_AND_WAIT)
     :
     "memory", "cc" L4S_PIC_CLOBBER
     );
  return rtag;
}


L4_INLINE l4_msgtag_t
l4_ipc_send(l4_cap_idx_t dest, l4_utcb_t *u,
	    l4_msgtag_t tag,
            l4_timeout_t timeout) L4_NOTHROW
{
  l4_umword_t dummy1, dummy4, dummy5;
  l4_msgtag_t rtag;

  (void)u;

  __asm__ __volatile__
    (L4_ENTER_KERNEL
     :
     "=d" (dummy1),
     "=a" (rtag.raw),
     "=c" (timeout.raw),
     "=S" (dummy4),
     "=D" (dummy5)
     :
     "S" (0),
     "c" (timeout),
     "d" (dest | L4_SYSF_SEND),
     "a" (tag.raw)
     : "memory", "cc" L4S_PIC_CLOBBER
     );
  return rtag;
}


L4_INLINE l4_msgtag_t
l4_ipc_wait(l4_utcb_t *u, l4_umword_t *label,
            l4_timeout_t timeout) L4_NOTHROW
{
  l4_umword_t dummy, dummy2, dummy1;
  l4_msgtag_t rtag;

  (void)u;

  __asm__ __volatile__
    (L4_ENTER_KERNEL
     :
     "=d" (dummy1),
     "=S" (*label),
     "=c" (dummy),
     "=D" (dummy2),
     "=a" (rtag.raw)
     :
     "c" (timeout),
     "d" (L4_INVALID_CAP | L4_SYSF_WAIT),
     "a" (0),
     "S" (0)
     : "memory", "cc"  L4S_PIC_CLOBBER
     );
  return rtag;
}

#if 0
L4_INLINE int
l4_ipc_wait_next_period(l4_threadid_t *src,
            l4_timeout_t timeout,
            l4_msgdope_t *result,
            l4_utcb_t *u)
{
  unsigned dummy, tag;

  (void)u;

  __asm__ __volatile__
    ("pushl %%ebp		\n\t" /* save ebp, no memory references
					 ("m") after this point */
     "movl  %[msg_desc], %%ebp	\n\t" /* rcv_msg */
     IPC_SYSENTER
     "popl  %%ebp		\n\t" /* restore ebp, no memory
					 references ("m") before this point */
     :
     "=a" (*result),
     "=d" (*rcv_dword0),
     "=b" (*rcv_dword1),
     "=c" (dummy),
     "=S" (src->raw),
     "=D" (tag)
     :
     "a" (L4_IPC_NIL_DESCRIPTOR),
     "c" (timeout),
     [msg_desc] "ir"(((int)rcv_msg) | L4_IPC_OPEN_IPC),
     "D" (L4_IPC_FLAG_NEXT_PERIOD) /* no absolute timeout */
     );
  return L4_IPC_ERROR(*result);
}
#endif

L4_INLINE l4_msgtag_t
l4_ipc_receive(l4_cap_idx_t rcv, l4_utcb_t *u,
               l4_timeout_t timeout) L4_NOTHROW
{
  l4_umword_t dummy, dummy1, dummy2;
  l4_msgtag_t rtag;

  (void)u;

  __asm__ __volatile__
    (L4_ENTER_KERNEL
     :
     "=d" (dummy2),
     "=S" (dummy1),
     "=c" (dummy),
     "=a" (rtag.raw)
     :
     "c" (timeout),
     "S" (0),
     "a" (0),
     "d" (rcv | L4_SYSF_RECV)
     : "memory", "cc"  L4S_PIC_CLOBBER
     );
  return rtag;
}

L4_INLINE l4_msgtag_t
l4_ipc(l4_cap_idx_t dest, l4_utcb_t *u,
       l4_umword_t flags,
       l4_umword_t slabel,
       l4_msgtag_t tag,
       l4_umword_t *rlabel,
       l4_timeout_t timeout) L4_NOTHROW
{
  l4_umword_t dummy, dummy1, dummy2;
  l4_msgtag_t rtag;

  (void)u;

  __asm__ __volatile__
    (L4_ENTER_KERNEL
     :
     "=d" (dummy2),
     "=S" (*rlabel),
     "=c" (dummy1),
     "=D" (dummy),
     "=a" (rtag.raw)
     :
     "S" (slabel),
     "c" (timeout),
     "a" (tag.raw),
     "d" (dest | flags)
     :
     "memory", "cc" L4S_PIC_CLOBBER
     );
  return rtag;
}

