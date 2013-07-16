/*
 * (c) 2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>
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
#ifndef __L4SYS__INCLUDE__ARCH_SPARC__KDEBUG_H__
#define __L4SYS__INCLUDE__ARCH_SPARC__KDEBUG_H__

#include <l4/sys/compiler.h>
#include <l4/sys/consts.h>
#ifdef __GNUC__

#ifndef L4_SYSCALL_MAGIC_OFFSET
#  define L4_SYSCALL_MAGIC_OFFSET	8
#endif
#define L4_SYSCALL_ENTER_KDEBUG		(-0x00000020-L4_SYSCALL_MAGIC_OFFSET)


#define enter_kdebug(text...) do { } while (1)

L4_INLINE void
outnstring(const char* x, unsigned len);

L4_INLINE void
outstring(const char *text);

L4_INLINE void
outchar(char c);

L4_INLINE void
outdec(int number);

L4_INLINE void
outhex32(int number);

L4_INLINE void
outhex20(int number);

L4_INLINE void
outhex16(int number);

L4_INLINE void
outhex12(int number);

L4_INLINE void
outhex8(int number);

L4_INLINE void
kd_display(char *text);

L4_INLINE int
l4kd_inchar(void);

L4_INLINE void
l4_kdebug_imb(void);

L4_INLINE void __touch_ro(const char *x, unsigned len);

L4_INLINE unsigned long
__kdebug_param(unsigned long nr, unsigned long p1, unsigned long p2);

L4_INLINE unsigned long
__kdebug_param_5(unsigned long nr, unsigned long p1, unsigned long p2,
                 unsigned long p3, unsigned long p4, unsigned long p5);

L4_INLINE
unsigned long
__kdebug_param(unsigned long nr, unsigned long p1, unsigned long p2)
{
	(void)nr;
	(void)p1;
	(void)p2;
	return -1;
}

L4_INLINE
unsigned long
__kdebug_param_5(unsigned long nr, unsigned long p1, unsigned long p2,
                 unsigned long p3, unsigned long p4, unsigned long p5)
{
	(void)nr;
	(void)p1;
	(void)p2;
	(void)p3;
	(void)p4;
	(void)p5;
	return -1;
}

L4_INLINE void
__touch_ro(const char *x, unsigned len)
{
   volatile const char *sptr, *eptr;
   sptr = (const char*)((unsigned)x & L4_PAGEMASK);
   eptr = (const char*)(((unsigned)x + len -1) & L4_PAGEMASK);

   for(;sptr <= eptr; sptr += L4_PAGESIZE)
     (void)(*sptr);
}

L4_INLINE void
outnstring(const char* x, unsigned len)
{
  __touch_ro(x, len);
  __kdebug_param(3, (unsigned long)x, (unsigned long)len);
}

L4_INLINE void
outstring(const char *text)
{
  unsigned i = 0;
  while(text[i++]) ;
  outnstring(text, i);
}

L4_INLINE void
outchar(char c)
{
  __kdebug_param(1, (unsigned long)c, 0);
}

L4_INLINE void
outdec(int number)
{
  __kdebug_param(4, (unsigned long)number, 0);
}

L4_INLINE void
outhex32(int number)
{
  __kdebug_param(5, (unsigned long)number, 0);
}

L4_INLINE void
outhex20(int number)
{
  __kdebug_param(6, (unsigned long)number, 0);
}

L4_INLINE void
outhex16(int number)
{
  __kdebug_param(7, (unsigned long)number, 0);
}

L4_INLINE void
outhex12(int number)
{
  __kdebug_param(8, (unsigned long)number, 0);
}

L4_INLINE void
outhex8(int number)
{
  __kdebug_param(9, (unsigned long)number, 0);
}

L4_INLINE void
kd_display(char *text)
{
  outstring(text);
}

L4_INLINE int
l4kd_inchar(void)
{
  return __kdebug_param(0xd, 0, 0);
}

L4_INLINE void
l4_kdebug_imb(void)
{
 // __KDEBUG_ARM_PARAM_0(0x3f);
}
#endif //__GNUC__

#endif /* ! __L4SYS__INCLUDE__ARCH_SPARC__KDEBUG_H__ */
