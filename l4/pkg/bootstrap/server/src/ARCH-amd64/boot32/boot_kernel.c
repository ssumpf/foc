/*
 * (c) 2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *          Alexander Warg <warg@os.inf.tu-dresden.de>,
 *          Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#include <stdio.h>
#include <l4/util/mb_info.h>
#include "boot_cpu.h"
#include "boot_paging.h"
#include "load_elf.h"

extern unsigned KERNEL_CS_64;
extern char _binary_bootstrap64_bin_start;

static l4_uint64_t find_upper_mem(l4util_mb_info_t *mbi)
{
  l4_uint64_t max = 0;
  l4util_mb_addr_range_t *mmap;
  l4util_mb_for_each_mmap_entry(mmap, mbi)
    {
      if (max < mmap->addr + mmap->size)
        max = mmap->addr + mmap->size;
    }
  return max;
}

void bootstrap (l4util_mb_info_t *mbi, unsigned int flag, char *rm_pointer);
void
bootstrap (l4util_mb_info_t *mbi, unsigned int flag, char *rm_pointer)
{
  l4_uint32_t vma_start, vma_end;
  struct
  {
    l4_uint32_t start;
    l4_uint16_t cs __attribute__((packed));
  } far_ptr;
  l4_uint64_t mem_upper;

  // setup stuff for base_paging_init()
  base_cpu_setup();

#ifdef REALMODE_LOADING
  mem_upper = *(unsigned long*)(rm_pointer + 0x1e0);
  mem_upper = 1024 * (1024 + mem_upper);
#else
  mem_upper = find_upper_mem(mbi);
  if (!mem_upper)
    mem_upper = 1024 * (1024 + mbi->mem_upper);
#endif

  printf("Highest physical memory address found: %llx\n", mem_upper);

  // now do base_paging_init(): sets up paging with one-to-one mapping
  base_paging_init(round_superpage(mem_upper));

  printf("Loading 64bit part...\n");
  // switch from 32 Bit compatibility mode to 64 Bit mode
  far_ptr.cs    = KERNEL_CS_64;
  far_ptr.start = load_elf(&_binary_bootstrap64_bin_start,
                           &vma_start, &vma_end);

  asm volatile("ljmp *(%4)"
                :: "D"(mbi), "S"(flag), "d"(rm_pointer),
                   "c"(&ptab64_mem_info),
                   "r"(&far_ptr), "m"(far_ptr));
}
