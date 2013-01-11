/**
 * \file
 * \brief Queue
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
#ifndef __L4UTIL_QUEUE_H__
#define __L4UTIL_QUEUE_H__

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>

EXTERN_C_BEGIN

struct l4util_buffer_head
{
  l4_threadid_t src;
  l4_uint32_t   len;
  char          *buffer;
};

L4_CV int l4util_queue_dequeue(struct l4util_buffer_head **buffer);

L4_CV int l4util_queue_init(int queue_threadno,
			    void *(*malloc_func)(l4_uint32_t size),
			    l4_uint32_t max_rcv);

EXTERN_C_END

#endif /* !__L4UTIL_QUEUE_H__ */
