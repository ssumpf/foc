INTERFACE [arm]:

#include "kmem.h"

class Page_table;

class Kmem_space : public Kmem
{
public:
  static void init();
  static void init_hw();
  static Page_table *kdir();

private:
  static Page_table *_kdir;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cassert>
#include <panic.h>

#include "console.h"
#include "pagetable.h"
#include "kmem.h"
#include "kip_init.h"
#include "mem_unit.h"

#include <cstdio>

char kernel_page_directory[sizeof(Page_table)] __attribute__((aligned(0x4000)));

Page_table *Kmem_space::_kdir = (Page_table*)&kernel_page_directory;

IMPLEMENT inline
Page_table *Kmem_space::kdir()
{ return _kdir; }

// initialze the kernel space (page table)
IMPLEMENT
void Kmem_space::init()
{
  Page_table::init();

  Mem_unit::clean_vdcache();
}

