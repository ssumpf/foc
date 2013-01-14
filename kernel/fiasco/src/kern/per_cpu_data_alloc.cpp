INTERFACE:

#include "per_cpu_data.h"

class Per_cpu_data_alloc : public Per_cpu_data
{
public:
  static bool alloc(unsigned cpu);
};


IMPLEMENTATION [!mp]:

IMPLEMENT inline
bool Per_cpu_data_alloc::alloc(unsigned)
{ return true; }


//---------------------------------------------------------------------------
IMPLEMENTATION [mp]:

#include <cstdio>
#include <cstring>

#include "config.h"
#include "kmem_alloc.h"

IMPLEMENT
bool Per_cpu_data_alloc::alloc(unsigned cpu)
{
  if (cpu >= Num_cpus || valid(cpu))
    return false;

  extern char _per_cpu_data_start[];
  extern char _per_cpu_data_end[];

  printf("Per_cpu_data_alloc: (orig: %p-%p)\n", _per_cpu_data_start, _per_cpu_data_end);

  if (cpu == 0)
    {
      // we use the master copy for CPU 0
      _offsets[0] = 0;
      return true;
    }

  unsigned size = _per_cpu_data_end - _per_cpu_data_start;

  char *per_cpu = (char*)Kmem_alloc::allocator()->unaligned_alloc(size);

  if (!per_cpu)
    return false;

  memset(per_cpu, 0, size);

  _offsets[cpu] = per_cpu - _per_cpu_data_start;
  printf("Allocate %u bytes (%uKB) for CPU[%u] local storage (offset=%lx, %p-%p)\n",
         size, (size + 513) / 1024, cpu, _offsets[cpu],
         _per_cpu_data_start + _offsets[cpu],
         _per_cpu_data_end + _offsets[cpu]);

  return true;

}

