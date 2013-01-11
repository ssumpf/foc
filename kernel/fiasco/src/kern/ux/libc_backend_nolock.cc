#include <libc_backend.h>

unsigned long __libc_backend_printf_lock()
{
  return 1;
}

void __libc_backend_printf_unlock(unsigned long)
{
}
