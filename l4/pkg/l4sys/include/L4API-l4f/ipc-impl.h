/**
 * \file
 * \brief Common IPC inline implementations.
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
#ifndef __L4SYS__INCLUDE__L4API_FIASCO__IPC_IMPL_H__
#define __L4SYS__INCLUDE__L4API_FIASCO__IPC_IMPL_H__


L4_INLINE l4_msgtag_t
l4_ipc_sleep(l4_timeout_t timeout) L4_NOTHROW
{ return l4_ipc_receive(L4_INVALID_CAP, NULL, timeout); }

#endif /* ! __L4SYS__INCLUDE__L4API_FIASCO__IPC_IMPL_H__ */
