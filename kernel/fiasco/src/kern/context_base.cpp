INTERFACE:

#include "types.h"
#include "config_tcbsize.h"

class Context;

class Context_base
{
public:
  enum
  {
    Size = THREAD_BLOCK_SIZE
  };

  // This virtual dtor enforces that Context / Thread / Context_base
  // all start at offset 0
  virtual ~Context_base() = 0;

protected:
  Mword _state;
};

//---------------------------------------------------------------------------
INTERFACE [mp]:

EXTENSION class Context_base
{
protected:
  friend unsigned &__cpu_of(const void *);
  unsigned _cpu;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [!mp]:

inline
void set_cpu_of(const void *ptr, unsigned cpu)
{ (void)ptr; (void)cpu; }

inline
unsigned cpu_of(const void *)
{ return 0; }

inline
unsigned current_cpu()
{ return 0; }

//---------------------------------------------------------------------------
IMPLEMENTATION:

#include "config.h"
#include "processor.h"

IMPLEMENT inline Context_base::~Context_base() {}

inline NEEDS ["config.h"]
Context *context_of(const void *ptr)
{
  return reinterpret_cast<Context *>
    (reinterpret_cast<unsigned long>(ptr) & ~(Context_base::Size - 1));
}

inline NEEDS [context_of, "processor.h"]
Context *current()
{ return context_of((void *)Proc::stack_pointer()); }

//---------------------------------------------------------------------------
IMPLEMENTATION [mp]:

#include "config.h"

inline NEEDS ["config.h"]
unsigned &__cpu_of(const void *ptr)
{ return reinterpret_cast<Context_base*>(context_of(ptr))->_cpu; }

inline NEEDS [__cpu_of]
void set_cpu_of(const void *ptr, unsigned cpu)
{ __cpu_of(ptr) = cpu; }


inline NEEDS [__cpu_of]
unsigned cpu_of(const void *ptr)
{ return __cpu_of(ptr); }

inline NEEDS [current, cpu_of]
unsigned current_cpu()
{ return cpu_of(current()); }

