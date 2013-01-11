INTERFACE:

#include "std_macros.h"

#ifdef NDEBUG
# define assert_kdb(expression) do {} while (0)
# define check_kdb(expr) (void)(expr)

#else /* ! NDEBUG */
# include <cstdio>
# define assert_kdb(expression) \
    do { if (EXPECT_FALSE(!(expression))) \
	  { printf("%s:%d: ASSERTION FAILED (%s)\n", __FILE__, __LINE__, #expression); \
	kdb_ke("XXX");} } while (0)
# define check_kdb(expr) assert_kdb(expr)
#endif /* ! NDEBUG */

