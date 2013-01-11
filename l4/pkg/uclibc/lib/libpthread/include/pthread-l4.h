#pragma once

#include <l4/sys/compiler.h>
#include <l4/sys/types.h>
#include <pthread.h>

__BEGIN_DECLS

l4_cap_idx_t pthread_getl4cap(pthread_t t);
void pthread_l4_for_each_thread(void (*fn)(pthread_t));

__END_DECLS
