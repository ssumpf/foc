INTERFACE:

#include <cstddef>		// size_t
#include "buddy_alloc.h"
#include "config.h"
#include "lock_guard.h"
#include "spin_lock.h"

#include "slab_cache.h"		// Slab_cache
#include <cxx/slist>

class Kmem_slab : public Slab_cache, public cxx::S_list_item
{
  friend class Jdb_kern_info_memory;
  typedef cxx::S_list_bss<Kmem_slab> Reap_list;

  // STATIC DATA
  static Reap_list reap_list;
};

template< typename T >
class Kmem_slab_t : public Kmem_slab
{
public:
  explicit Kmem_slab_t(char const *name)
  : Kmem_slab(sizeof(T), __alignof(T), name) {}
};

IMPLEMENTATION:

Kmem_slab::Reap_list Kmem_slab::reap_list;

// Kmem_slab -- A type-independent slab cache allocator for Fiasco,
// derived from a generic slab cache allocator (Slab_cache in
// lib/slab.cpp).

// This specialization adds low-level page allocation and locking to
// the slab allocator implemented in our base class (Slab_cache).
//-

#include <cassert>
#include "config.h"
#include "atomic.h"
#include "panic.h"
#include "kmem_alloc.h"

// Specializations providing their own block_alloc()/block_free() can
// also request slab sizes larger than one page.
PROTECTED
Kmem_slab::Kmem_slab(unsigned long slab_size,
				   unsigned elem_size,
				   unsigned alignment,
				   char const *name)
  : Slab_cache(slab_size, elem_size, alignment, name)
{
  reap_list.add(this, mp_cas<cxx::S_list_item*>);
}

// Specializations providing their own block_alloc()/block_free() can
// also request slab sizes larger than one page.
PUBLIC
Kmem_slab::Kmem_slab(unsigned elem_size,
                     unsigned alignment,
                     char const *name,
                     unsigned long min_size = Buddy_alloc::Min_size,
                     unsigned long max_size = Buddy_alloc::Max_size)
  : Slab_cache(elem_size, alignment, name, min_size, max_size)
{
  reap_list.add(this, mp_cas<cxx::S_list_item*>);
}

PUBLIC
Kmem_slab::~Kmem_slab()
{
  destroy();
}


// Callback functions called by our super class, Slab_cache, to
// allocate or free blocks

virtual void *
Kmem_slab::block_alloc(unsigned long size, unsigned long)
{
  assert (size >= Buddy_alloc::Min_size);
  assert (size <= Buddy_alloc::Max_size);
  assert (!(size & (size - 1)));
  (void)size;
  return Kmem_alloc::allocator()->unaligned_alloc(size);
}

virtual void
Kmem_slab::block_free(void *block, unsigned long size)
{
  Kmem_alloc::allocator()->unaligned_free(size, block);
}

// 
// Memory reaper
// 
PUBLIC static
size_t
Kmem_slab::reap_all (bool desperate)
{
  size_t freed = 0;

  for (Reap_list::Const_iterator alloc = reap_list.begin();
       alloc != reap_list.end(); ++alloc)
    {
      size_t got;
      do
	{
	  got = alloc->reap();
	  freed += got;
	}
      while (desperate && got);
    }

  return freed;
}

static Kmem_alloc_reaper kmem_slab_reaper(Kmem_slab::reap_all);
