#ifndef _ASSERT_H
#define _ASSERT_H

#include <cdefs.h>
#include <fiasco_defs.h>

#if (__GNUC__>=3)
#  define ASSERT_EXPECT_FALSE(exp)	__builtin_expect((exp), 0)
#else
#  define ASSERT_EXPECT_FALSE(exp)	(exp)
#endif

#if defined ASSERT_KDB_KE
#  include "kdb_ke.h"
#  define assert(expr)						\
   do { if (ASSERT_EXPECT_FALSE(!(expr)))			\
	  kdb_ke(#expr); } while(0)
#  define check(expr) assert(expr)
#else
__BEGIN_DECLS
/* This prints an "Assertion failed" message and aborts.  */
void FIASCO_COLD __assert_fail (const char *__assertion, const char *__file,
		    unsigned int __line, void *ret)
     __attribute__ ((__noreturn__));

__END_DECLS


/* We don't show information about the current function since it needs
 * additional space -- especially with gcc-2.95. The function name
 * can be found by searching the EIP in the kernel image. */
#  undef assert
#  ifdef NDEBUG
#    define assert(expr)  do {} while (0)
#    define check(expr) (void)(expr)
#  else
#    define assert(expr)						\
     do { if (ASSERT_EXPECT_FALSE(!(expr))) __assert_fail (#expr, __FILE__, __LINE__, __builtin_return_address(0)); } while (0)
#  endif
#endif

#endif
