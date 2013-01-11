/**
 * \file
 * \brief ARM specific implementation of bitops functions
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */
#ifndef __L4UTIL__ARCH_ARM__BITOPS_ARCH_H__
#define __L4UTIL__ARCH_ARM__BITOPS_ARCH_H__

/* No optimized variants available. */

L4_INLINE void
l4util_set_bit32(int b, volatile l4_uint32_t * dest)
{
  // for ARM sizeof(l4_umword_t) == sizeof(l4_uint32_t)
  l4util_set_bit(b, (volatile l4_umword_t *)dest);
}

#endif /* ! __L4UTIL__ARCH_ARM__BITOPS_ARCH_H__ */
