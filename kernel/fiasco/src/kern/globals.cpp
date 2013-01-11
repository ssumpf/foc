INTERFACE:

#include <cassert>

#include "panic.h"
#include "per_cpu_data.h"
#include "types.h"

class Timeout;

extern Per_cpu<Timeout *> timeslice_timeout;

/* the check macro is like assert(), but it evaluates its argument
   even if NDEBUG is defined */
#ifndef check
#ifdef NDEBUG
# define check(expression) ((void)(expression))
#else /* ! NDEBUG */
# ifdef ASSERT_KDB_KE
#  define check(expression) assert(expression)
# else
#  define check(expression) \
          ((void)((expression) ? 0 : \
                 (panic(__FILE__":%u: failed check `"#expression"'", \
                         __LINE__), 0)))
# endif
#endif /* ! NDEBUG */
#endif /* check */

class Kobject_iface;

class Initial_kobjects
{
public:
  enum
  {
    Max = 5,
    First_cap = 5,

    End_cap = First_cap + Max,
  };

  void register_obj(Kobject_iface *o, unsigned cap)
  {
    assert (cap >= First_cap);
    assert (cap < End_cap);

    cap -= First_cap;

    assert (!_v[cap]);

    _v[cap] = o;
  }

  Kobject_iface *obj(unsigned cap) const
  {
    assert (cap >= First_cap);
    assert (cap < End_cap);

    cap -= First_cap;

    return _v[cap];
  }

private:
  Kobject_iface *_v[Max];
};


extern Initial_kobjects initial_kobjects;


//---------------------------------------------------------------------------
IMPLEMENTATION:

Initial_kobjects initial_kobjects;


