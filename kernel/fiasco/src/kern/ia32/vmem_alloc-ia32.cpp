IMPLEMENTATION[ia32,ux,amd64]:

#include <cassert>
#include <cstdio>
#include <cstring>
#include "config.h"
#include "kdb_ke.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "mem_layout.h"
#include "mem_unit.h"
#include "paging.h"
#include "static_init.h"
#include "initcalls.h"
#include "space.h"

IMPLEMENT
void*
Vmem_alloc::page_alloc(void *address, Zero_fill zf, unsigned mode)
{
  void *vpage = 0;
  Address page;

  vpage = Kmem_alloc::allocator()->alloc(Config::PAGE_SHIFT);

  if (EXPECT_FALSE(!vpage))
    return 0;

  // insert page into master page table
  Pdir::Iter e = Kmem::kdir->walk(Virt_addr(address), 100,
                                  pdir_alloc(Kmem_alloc::allocator()));
  if (EXPECT_FALSE(e.e->valid()))
    {
      kdb_ke("page_alloc: address already mapped");
      goto error;
    }

  if (e.shift() != Config::PAGE_SHIFT)
    goto error;

  if (zf == ZERO_FILL)
    memset(vpage, 0, Config::PAGE_SIZE);

  page = Mem_layout::pmem_to_phys((Address)vpage);

  *e.e = page | Pt_entry::Writable | Pt_entry::Dirty
    | Pt_entry::Valid | Pt_entry::Referenced | Pt_entry::global();
  page_map (address, 0, zf, page);

  if (mode & User)
    e.e->add_attr(Pt_entry::User);

  return address;

error:
  Kmem_alloc::allocator()->free(Config::PAGE_SHIFT, vpage); // 2^0 = 1 page
  return 0;
}


//----------------------------------------------------------------------------
IMPLEMENTATION[ia32,amd64]:

IMPLEMENT inline
void
Vmem_alloc::page_map(void * /*address*/, int /*order*/, Zero_fill /*zf*/,
		     Address /*phys*/)
{}

