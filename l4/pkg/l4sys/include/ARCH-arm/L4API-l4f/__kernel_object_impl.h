/**
 * \file
 * \brief  Low-level kernel functions for ARM
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
#ifndef L4SYS_KERNEL_OBJECT_IMPL_H__
#define L4SYS_KERNEL_OBJECT_IMPL_H__

#include <l4/sys/syscall_defs.h>

L4_INLINE l4_msgtag_t
l4_invoke_debugger(l4_cap_idx_t obj, l4_msgtag_t tag, l4_utcb_t *utcb) L4_NOTHROW
{
  register unsigned long _obj __asm__ ("r2") = obj;
  register unsigned long _a1  __asm__ ("r0") = tag.raw;
  register void* _a4  __asm__ ("r1") = utcb;

  __asm__ __volatile__
    ("@ l4_syscall_debugger(start) \n\t"
     "mov     lr, pc	     \n\t"
     "mov     pc, %[sc]	     \n\t"
     "@ l4_syscall_debugger(end)   \n\t"
       :
	"=r" (_obj),
	"=r" (_a1),
	"=r" (_a4)
       :
       [sc] "i" (L4_SYSCALL_DEBUGGER),
	"0" (_obj),
	"1" (_a1),
	"2" (_a4)
       :
	"cc", "memory", "lr"
       );

  tag.raw = _a1; // gcc doesn't like the return out of registers variables
  return tag;
}

#endif
