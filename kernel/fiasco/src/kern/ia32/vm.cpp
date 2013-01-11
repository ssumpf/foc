INTERFACE:

#include "task.h"

class Vm : public Task
{
public:
  explicit Vm(Ram_quota *q) : Task(q) {}
  int resume_vcpu(Context *, Vcpu_state *, bool) = 0;
};

template< typename VM >
struct Vm_allocator
{
  static Kmem_slab_t<VM> a;
};

template<typename VM>
Kmem_slab_t<VM> Vm_allocator<VM>::a("Vm");

// ------------------------------------------------------------------------
IMPLEMENTATION:

PUBLIC inline virtual
Page_number
Vm::map_max_address() const
{ return Page_number::create(1UL << (MWORD_BITS - Mem_space::Page_shift)); }

PUBLIC static
template< typename VM >
Slab_cache *
Vm::allocator()
{ return &Vm_allocator<VM>::a; }

// ------------------------------------------------------------------------
IMPLEMENTATION [ia32]:

PROTECTED static inline
bool
Vm::is_64bit()
{ return false; }

// ------------------------------------------------------------------------
IMPLEMENTATION [amd64]:

PROTECTED static inline
bool
Vm::is_64bit()
{ return true; }
