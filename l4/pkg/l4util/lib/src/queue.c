/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *               Michael Hohmuth <hohmuth@os.inf.tu-dresden.de>,
 *               Lars Reuther <reuther@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */

/* IPC message queueing functions */

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/util/lock.h>
#include <l4/util/queue.h>

#define STACK_SIZE 512

l4_threadid_t l4util_queue_id;

static void *(*queue_malloc)(l4_uint32_t size);
static l4_uint32_t max_rcv_size;

struct buffer
{
  struct l4util_buffer_head head;
  struct buffer *next, *prev;
  l4_umword_t d1, d2;		/* must come last - just before the
				   string buffer */
};

static struct buffer *q_head = 0, *q_tail = 0;
static l4util_simple_lock_t q_lock = 0;

/* dequeue a message queued by the queue thread */
int
l4util_queue_dequeue(struct l4util_buffer_head **buffer)
{
  l4_simple_lock(&q_lock);

  if ((*buffer = (struct l4util_buffer_head *) q_head) != 0)
    if ((q_head = q_head->next) == 0)
      {
	q_tail = 0;
      }

  l4_simple_unlock(&q_lock);
  
  return *buffer ? 0 : -1;
} 

/* the queue thread: it takes messages from any sender and queues them. */

static void queue_thread(void)
{ 
  static struct 
    {
      l4_fpage_t fpage;
      l4_msgdope_t dope;
      l4_msgdope_t snd_dope;
      l4_umword_t d1, d2;
      l4_strdope_t str;
    } rcv_buffer 
  = 
    {
	{fpage: 0}, 
	{md: {0, 0, 0, 0, 0, 0, 1, 0}}, 
	{msgdope: 0}, 
	0, 0, {0, 0, 0, 0}
    };

  int error;
  l4_msgdope_t result;

  rcv_buffer.str.rcv_size = max_rcv_size - 8; /* 2 mandantory dwords = 8 byte */

  for (;;)
    {
      struct buffer *b = queue_malloc(max_rcv_size - 8
				      + sizeof(struct buffer));
      rcv_buffer.str.rcv_str = (l4_umword_t) &b->d1;
      b->head.buffer         = (char *)      &b->d1;

      error = l4_ipc_wait(&b->head.src, &rcv_buffer,
			       &b->d1, &b->d2,
			       L4_IPC_NEVER, &result);

      if (error)
	continue;

      b->head.len = rcv_buffer.str.snd_size;

      l4_simple_lock(&q_lock);

      b->next = 0;
      if (! q_tail)
	q_head = b;
      else
	q_tail->next = b;
      q_tail = b;

      l4_simple_unlock(&q_lock);
    }
}


/* initialization */

int
l4util_queue_init(int queue_threadno,
		  void *(*malloc_func)(l4_uint32_t size),
		  l4_uint32_t max_rcv)
{
  char *th_stack;
  l4_umword_t dummy;
  l4_threadid_t my_preempter, my_pager;
  l4_threadid_t me;

  queue_malloc = malloc_func;
  max_rcv_size = max_rcv;

  th_stack = queue_malloc(STACK_SIZE);
  if (! th_stack)
    return -1;

  me = l4_myself();
  my_preempter = L4_INVALID_ID;
  my_pager = L4_INVALID_ID;
  l4_thread_ex_regs(me, (l4_umword_t) -1, (l4_umword_t) -1,
                    &my_preempter,
                    &my_pager,
                    &dummy,
                    &dummy,
                    &dummy);
  l4util_queue_id = me;
  l4util_queue_id.id.lthread = queue_threadno;

  l4_thread_ex_regs(l4util_queue_id, (l4_umword_t) queue_thread, 
                    ((l4_umword_t) th_stack) + STACK_SIZE,
                    &my_preempter,
                    &my_pager,
                    &dummy,
                    &dummy,
                    &dummy);

  return 0;
}

