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
  friend Cpu_number &__cpu_of(const void *);
  Mword _state;
  Cpu_number _cpu;
};


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

inline NEEDS ["config.h"]
Cpu_number &__cpu_of(const void *ptr)
{ return reinterpret_cast<Context_base*>(context_of(ptr))->_cpu; }

inline NEEDS [__cpu_of]
void set_cpu_of(const void *ptr, Cpu_number cpu)
{ __cpu_of(ptr) = cpu; }


inline NEEDS [__cpu_of]
Cpu_number cpu_of(const void *ptr)
{ return __cpu_of(ptr); }

//---------------------------------------------------------------------------
IMPLEMENTATION [!mp]:

inline
Cpu_number current_cpu()
{ return Cpu_number::boot_cpu(); }

//---------------------------------------------------------------------------
IMPLEMENTATION [mp]:

inline NEEDS [current, cpu_of]
Cpu_number current_cpu()
{ return cpu_of(current()); }

