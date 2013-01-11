INTERFACE:

class Jdb_util
{
public:
  static bool is_mapped(void const *addr);
};

IMPLEMENTATION[ia32|ux|amd64]:

#include "kmem.h"

IMPLEMENT
bool
Jdb_util::is_mapped(void const *x)
{
  return Kmem::virt_to_phys(x) != ~0UL;
}

IMPLEMENTATION[arm]:

#include "pagetable.h"
#include "kmem_space.h"

IMPLEMENT inline NEEDS["kmem_space.h", "pagetable.h"]
bool
Jdb_util::is_mapped(void const* addr)
{
  return Kmem_space::kdir()->walk(const_cast<void*>(addr), 0, false, Ptab::Null_alloc(), 0).valid();
}

IMPLEMENTATION[ppc32]:

IMPLEMENT inline
bool
Jdb_util::is_mapped(void const * /*addr*/)
{
  return true;
}

IMPLEMENTATION[sparc]:

IMPLEMENT inline
bool
Jdb_util::is_mapped(void const * /*addr*/)
{
  return false; // TBD
}
