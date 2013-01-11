#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <panic.h>

#include "kernel_console.h"
#include "simpleio.h"
#include "terminate.h"


void
__assert_fail (const char *__assertion, const char *__file,
	       unsigned int __line)
{
  // make sure that GZIP mode is off
  Kconsole::console()->end_exclusive(Console::GZIP);

  printf("\nAssertion failed: '%s'\n"
	 "  in %s:%i\n"
	 "  at " L4_PTR_FMT "\n",
	 __assertion, __file, __line, (Address)__builtin_return_address(0));

  terminate(1);
}

void
panic(const char *format, ...)
{
  // make sure that GZIP mode is off
  Kconsole::console()->end_exclusive(Console::GZIP);

  va_list args;

  putstr("\033[1mPanic: ");
  va_start (args, format);
  vprintf  (format, args);
  va_end   (args);
  putstr("\033[m");

  terminate (EXIT_FAILURE);
}
