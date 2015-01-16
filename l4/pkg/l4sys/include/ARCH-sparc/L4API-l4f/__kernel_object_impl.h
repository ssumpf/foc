/**
 * \file
 * \brief  Low-level kernel functions for SPARC
 */
/*
 * (c) 2010    Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *             Björn Döbel <doebel@os.inf.tu-dresden.de>
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

#ifndef L4_SYSCALL_MAGIC_OFFSET
#  define L4_SYSCALL_MAGIC_OFFSET	8
#endif
#define L4_SYSCALL_DEBUGGER		(-0x0000000C-L4_SYSCALL_MAGIC_OFFSET)


L4_INLINE l4_msgtag_t
l4_invoke_debugger(l4_cap_idx_t obj, l4_msgtag_t tag, l4_utcb_t *utcb) L4_NOTHROW
{
	l4_msgtag_t t;
	(void)obj;
	(void)tag;
	(void)utcb;
	t.raw = ~0;
	return t;
}

