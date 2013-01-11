#include <libc_backend.h>

#include <spin_lock.h>

static Spin_lock<> __libc_backend_printf_spinlock;

unsigned long __libc_backend_printf_lock()
{
  return __libc_backend_printf_spinlock.test_and_set();
}

void __libc_backend_printf_unlock(unsigned long state)
{
  __libc_backend_printf_spinlock.set(state);
}
