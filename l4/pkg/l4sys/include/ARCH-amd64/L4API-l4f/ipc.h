/**
 * \file
 * \brief   L4 IPC System Calls, amd64
 * \ingroup api_calls
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *               Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
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
#ifndef __L4SYS__INCLUDE__ARCH_AMD64__L4API_L4F__IPC_H__
#define __L4SYS__INCLUDE__ARCH_AMD64__L4API_L4F__IPC_H__

#include <l4/sys/types.h>

#define L4_IPC_IOMAPMSG_BASE  0xfffffffff0000000 ///< I/O mapmsg base
#define L4_IPC_CAPMAPMSG_BASE 0xfffffffff0000100 ///< Capability mapmsg base

#include_next <l4/sys/ipc.h>

#include <l4/sys/ipc-invoke.h>

#ifdef PROFILE
#  include "ipc-l42-profile.h"
#else
#  include "ipc-l42-gcc3.h"
#endif

#include <l4/sys/ipc-impl.h>

#endif /* ! __L4SYS__INCLUDE__ARCH_AMD64__L4API_L4F__IPC_H__ */
