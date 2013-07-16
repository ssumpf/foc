/**
 * \file
 * \brief   Kernel debugger macros
 * \ingroup api_calls
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
#pragma once

#include <l4/sys/compiler.h>

#ifdef __GNUC__

#ifndef L4_SYSCALL_MAGIC_OFFSET
#  define L4_SYSCALL_MAGIC_OFFSET	8
#endif
#define L4_SYSCALL_ENTER_KDEBUG		(-0x00000020-L4_SYSCALL_MAGIC_OFFSET)

#define enter_kdebug(text...)			\
__asm__ __volatile__ (				\
    "	mov	lr, pc		\n"		\
    "	mov	pc, %0		\n"		\
    "	b	1f		\n"		\
    "	.ascii	\"" text "\"	\n"		\
    "	.byte	0		\n"		\
    "	.align	4		\n"		\
    "1:				\n"		\
    :						\
    : "i" (L4_SYSCALL_ENTER_KDEBUG)		\
    : "lr")

L4_INLINE void
outnstring(const char* x, unsigned len) L4_NOTHROW;

L4_INLINE void
outstring(const char *text) L4_NOTHROW;

L4_INLINE void
outchar(char c) L4_NOTHROW;

L4_INLINE void
outdec(int number) L4_NOTHROW;

L4_INLINE void
outhex32(int number) L4_NOTHROW;

L4_INLINE void
outhex20(int number) L4_NOTHROW;

L4_INLINE void
outhex16(int number) L4_NOTHROW;

L4_INLINE void
outhex12(int number) L4_NOTHROW;

L4_INLINE void
outhex8(int number) L4_NOTHROW;

L4_INLINE void
kd_display(char *text) L4_NOTHROW;

L4_INLINE int
l4kd_inchar(void) L4_NOTHROW;

/*
 * -------------------------------------------------------------------
 * Implementations
 */

#define __KDEBUG_ARM_PARAM_0(nr)				\
  ({								\
    register unsigned long r0 __asm__("r0");			\
    __asm__ __volatile__					\
      (								\
       "stmdb sp!, {r1-r12,lr}	\n\t"				\
       "mov	lr, pc		\n\t"				\
       "mov	pc, %1		\n\t"				\
       "cmp	lr, #" #nr "	\n\t"				\
       "ldmia sp!, {r1-r12,lr}	\n\t"				\
       :							\
       "=r" (r0)						\
       :							\
       "i" (L4_SYSCALL_ENTER_KDEBUG)				\
      );							\
    r0;								\
  })

#define __KDEBUG_ARM_PARAM_1(nr, p1)				\
  ({								\
    register unsigned long r0 __asm__("r0") = (unsigned long)(p1);\
    __asm__ __volatile__					\
      (								\
       "stmdb sp!, {r1-r12,lr}	\n\t"				\
       "mov	lr, pc		\n\t"				\
       "mov	pc, %1		\n\t"				\
       "cmp	lr, #" #nr "	\n\t"				\
       "ldmia sp!, {r1-r12,lr}	\n\t"				\
       :							\
       "=r" (r0)						\
       :							\
       "i" (L4_SYSCALL_ENTER_KDEBUG),				\
       "0" (r0)							\
      );							\
    r0;								\
  })

#define __KDEBUG_ARM_PARAM_2(nr, p1, p2)			\
  ({								\
    register unsigned long r0 __asm__("r0") = (unsigned long)(p1);\
    register unsigned long r1 __asm__("r1") = (unsigned long)(p2);\
    __asm__ __volatile__					\
      (								\
       "stmdb sp!, {r2-r12,lr}	\n\t"				\
       "mov	lr, pc		\n\t"				\
       "mov	pc, %2		\n\t"				\
       "cmp	lr, #" #nr "	\n\t"				\
       "ldmia sp!, {r2-r12,lr}	\n\t"				\
       :							\
       "=r" (r0),						\
       "=r" (r1)						\
       :							\
       "i" (L4_SYSCALL_ENTER_KDEBUG),				\
       "0" (r0),						\
       "r" (r1)							\
      );							\
    r0;								\
  })

#define __KDEBUG_ARM_PARAM_3(nr, p1, p2, p3)			\
  ({								\
    register unsigned long r0 __asm__("r0") = (unsigned long)(p1);\
    register unsigned long r1 __asm__("r1") = (unsigned long)(p2);\
    register unsigned long r2 __asm__("r2") = (unsigned long)(p3);\
    __asm__ __volatile__					\
      (								\
       "stmdb sp!, {r3-r12,lr}	\n\t"				\
       "mov	lr, pc		\n\t"				\
       "mov	pc, %3		\n\t"				\
       "cmp	lr, #" #nr "	\n\t"				\
       "ldmia sp!, {r3-r12,lr}	\n\t"				\
       :							\
       "=r" (r0),						\
       "=r" (r1),						\
       "=r" (r2)						\
       :							\
       "i" (L4_SYSCALL_ENTER_KDEBUG),				\
       "0" (r0),						\
       "r" (r1),						\
       "r" (r2)							\
      );							\
    r0;								\
  })

#define __KDEBUG_ARM_PARAM_5(nr, p1, p2, p3, p4, p5)		\
  ({								\
    register unsigned long r0 __asm__("r0") = (unsigned long)(p1);\
    register unsigned long r1 __asm__("r1") = (unsigned long)(p2);\
    register unsigned long r2 __asm__("r2") = (unsigned long)(p3);\
    register unsigned long r3 __asm__("r3") = (unsigned long)(p4);\
    register unsigned long r4 __asm__("r4") = (unsigned long)(p5);\
    __asm__ __volatile__					\
      (								\
       "stmdb sp!, {r5-r12,lr}	\n\t"				\
       "mov	lr, pc		\n\t"				\
       "mov	pc, %5		\n\t"				\
       "cmp	lr, #" #nr "	\n\t"				\
       "ldmia sp!, {r5-r12,lr}	\n\t"				\
       :							\
       "=r" (r0),						\
       "=r" (r1),						\
       "=r" (r2),						\
       "=r" (r3),						\
       "=r" (r4)						\
       :							\
       "i" (L4_SYSCALL_ENTER_KDEBUG),				\
       "0" (r0),						\
       "r" (r1),						\
       "r" (r2),						\
       "r" (r3),						\
       "r" (r4)							\
      );							\
    r0;								\
  })


L4_INLINE void
outnstring(const char* x, unsigned len) L4_NOTHROW
{
  __asm__ volatile ("" : : : "memory");
  __KDEBUG_ARM_PARAM_2(3, x, len);
}

L4_INLINE void
outstring(const char *text) L4_NOTHROW
{
  __asm__ volatile("" : : : "memory");
  __KDEBUG_ARM_PARAM_1(2, text);
}

L4_INLINE void
outchar(char c) L4_NOTHROW
{
  __KDEBUG_ARM_PARAM_1(1, c);
}

L4_INLINE void
outdec(int number) L4_NOTHROW
{
  __KDEBUG_ARM_PARAM_1(4, number);
}

L4_INLINE void
outhex32(int number) L4_NOTHROW
{
  __KDEBUG_ARM_PARAM_1(5, number);
}

L4_INLINE void
outhex20(int number) L4_NOTHROW
{
  __KDEBUG_ARM_PARAM_1(6, number);
}

L4_INLINE void
outhex16(int number) L4_NOTHROW
{
  __KDEBUG_ARM_PARAM_1(7, number);
}

L4_INLINE void
outhex12(int number) L4_NOTHROW
{
  __KDEBUG_ARM_PARAM_1(8, number);
}

L4_INLINE void
outhex8(int number) L4_NOTHROW
{
  __KDEBUG_ARM_PARAM_1(9, number);
}

L4_INLINE void
kd_display(char *text) L4_NOTHROW
{
  outstring(text);
}

L4_INLINE int
l4kd_inchar(void) L4_NOTHROW
{
  return __KDEBUG_ARM_PARAM_0(0xd);
}

#endif //__GNUC__
