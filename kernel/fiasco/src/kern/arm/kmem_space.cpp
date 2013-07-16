INTERFACE [arm]:

#include "paging.h"
#include "mem_layout.h"

class Kmem_space : public Mem_layout
{
public:
  static void init();
  static void init_hw();
  static Pdir *kdir();

private:
  static Pdir *_kdir;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cassert>
#include <panic.h>

#include "console.h"
#include "kip_init.h"
#include "mem_unit.h"

#include <cstdio>

IMPLEMENT inline
Pdir *Kmem_space::kdir()
{ return _kdir; }

// initialze the kernel space (page table)
IMPLEMENT
void Kmem_space::init()
{
  unsigned domains      = 0x0001;

  asm volatile (
      "mcr p15, 0, %0, c3, c0       \n" // domains
      :
      : "r"(domains) );

  Mem_unit::clean_vdcache();
}

// allways 16kB also for LPAE we use 4 consecutive second level tables
char kernel_page_directory[0x4000] __attribute__((aligned(0x4000), section(".bss.kernel_page_dir")));

//----------------------------------------------------------------------------------
IMPLEMENTATION[arm && !arm_lpae]:

Pdir *Kmem_space::_kdir = (Pdir*)&kernel_page_directory;

//----------------------------------------------------------------------------------
IMPLEMENTATION[arm && arm_lpae]:

Unsigned64 kernel_lpae_dir[4] __attribute__((aligned(4*sizeof(Unsigned64))));
Pdir *Kmem_space::_kdir = (Pdir*)&kernel_lpae_dir;



