/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#ifndef __BOOTSTRAP__LOADER_MBI_H__
#define __BOOTSTRAP__LOADER_MBI_H__

#include <l4/util/mb_info.h>

l4util_mb_info_t *init_loader_mbi(void *);

#endif /* ! __BOOTSTRAP__LOADER_MBI_H__ */
