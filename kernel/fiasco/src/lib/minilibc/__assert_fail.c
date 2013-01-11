#include "assert.h"
#include "stdio.h"
#include "stdlib.h"

void
__assert_fail (const char *__assertion, const char *__file,
	       unsigned int __line, void *ret)
{
  printf("ASSERTION_FAILED (%s)[ret=%p]\n"
	 "  in file %s:%d\n", __assertion, ret, __file, __line);
  exit (EXIT_FAILURE);
}
