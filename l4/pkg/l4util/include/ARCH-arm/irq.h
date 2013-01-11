/**
 * \file
 * \brief ARM specific implementation of irq functions
 *
 * Do not use.
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */
#ifndef __L4UTIL__ARCH_ARCH__IRQ_H__
#define __L4UTIL__ARCH_ARCH__IRQ_H__

#ifdef __GNUC__

#include <l4/sys/kdebug.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

L4_INLINE void l4util_cli (void);
L4_INLINE void l4util_sti (void);
L4_INLINE void l4util_flags_save(l4_umword_t *flags);
L4_INLINE void l4util_flags_restore(l4_umword_t *flags);

/** \brief Disable all interrupts
 * \internal
 */
L4_INLINE
void
l4util_cli (void)
{
  l4_sys_cli();
}

/** \brief Enable all interrupts
 * \internal
 */
L4_INLINE
void
l4util_sti (void)
{
  l4_sys_sti();
}

/**
 * \brief Do not use
 * \internal
 *
 * !!!!!!!
 *  We probably need some primitive like in linux here which
 *    enable/disable interrupts on l4util_flags_restore
 *
 */

L4_INLINE
void
l4util_flags_save(l4_umword_t *flags)
{
  (void)flags;
  enter_kdebug("l4util_flags_save");
}

/** \brief Restore processor flags. Can be used to restore the interrupt flag
 * \internal
 */
L4_INLINE
void
l4util_flags_restore(l4_umword_t *flags)
{
  (void)flags;
  enter_kdebug("l4util_flags_restore");
}

EXTERN_C_END

#endif //__GNUC__

#endif /* ! __L4UTIL__ARCH_ARCH__IRQ_H__ */
