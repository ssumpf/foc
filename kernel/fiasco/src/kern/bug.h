#pragma once

#ifdef NDEBUG

# define BUG_ON(expr, ...)

#else

# define BUG_ON(expr, ...) \
if (EXPECT_FALSE(!!(expr))) \
  { \
    printf("%s:%d: BUG (%s): ", __FILE__, __LINE__, #expr); \
    printf(__VA_ARGS__); kdb_ke("bug"); \
  }

#endif

