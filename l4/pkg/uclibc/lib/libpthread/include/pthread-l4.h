#pragma once

#include <l4/sys/compiler.h>
#include <l4/sys/types.h>
#include <pthread.h>

enum
{
  PTHREAD_L4_ATTR_NO_START = 0x0001,
};

__BEGIN_DECLS

l4_cap_idx_t pthread_getl4cap(pthread_t t);
void pthread_l4_for_each_thread(void (*fn)(pthread_t));

__END_DECLS

#ifdef __cplusplus

#include <l4/sys/thread>
inline L4::Cap<L4::Thread> pthread_l4_getcap(pthread_t t)
{ return L4::Cap<L4::Thread>(pthread_getl4cap(t)); }

#endif
