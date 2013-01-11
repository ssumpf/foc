#pragma once

#include <l4/sys/irq>

typedef L4::Irq Th_sem_cap;

inline int __alloc_thread_sem(pthread_descr th, L4::Cap<Th_sem_cap> const &c)
{
  int err = l4_error(L4Re::Env::env()->factory()->create_irq(c));
  if (err < 0)
    return err;

  err = l4_error(c->attach(16, L4::Cap<L4::Thread>(th->p_th_cap)));
  return err;
}
