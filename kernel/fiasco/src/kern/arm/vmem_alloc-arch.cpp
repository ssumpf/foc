//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "mem.h"
#include "pagetable.h"
#include "mem_space.h"
#include "kmem_alloc.h"
#include "config.h"
#include "panic.h"
#include "kdb_ke.h"
#include "mem_unit.h"
#include "ram_quota.h"
#include "static_init.h"

#include <cstdio>
#include <cassert>
#include <cstring>


IMPLEMENT
void *Vmem_alloc::page_alloc(void *address, Zero_fill zf, unsigned mode)
{
  void *vpage = Kmem_alloc::allocator()->alloc(Config::PAGE_SHIFT);

  if (EXPECT_FALSE(!vpage))
    return 0;

  Address page = Mem_space::kernel_space()->virt_to_phys((Address)vpage);
  //printf("  allocated page (virt=%p, phys=%08lx\n", vpage, page);
  Mem_unit::inv_dcache(vpage, ((char*)vpage) + Config::PAGE_SIZE);

  // insert page into master page table
  Pte pte = Mem_space::kernel_space()->dir()->walk(address, Config::PAGE_SIZE, true,
                                                   Kmem_alloc::q_allocator(Ram_quota::root),
                                                   Mem_space::kernel_space()->dir());

  unsigned long pa = Page::CACHEABLE;
  if (mode & User)
    pa |= Page::USER_RW;
  else
    pa |= Page::KERN_RW;

  pte.set(page, Config::PAGE_SIZE, Mem_page_attr(pa), true);

  Mem_unit::dtlb_flush(address);

  if (zf == ZERO_FILL)
    Mem::memset_mwords((unsigned long *)address, 0, Config::PAGE_SIZE >> 2);

  return address;
}

