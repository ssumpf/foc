/**
 * \file
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "init_kip.h"
#include "startup.h"
#include "region.h"
#include <l4/sys/kip>
#include <l4/sys/l4int.h>

using L4::Kip::Mem_desc;

/* XXX not possible to include kip.h from L4Ka::Pistachio here */

#if L4_MWORD_BITS == 32
#define V4KIP_SIGMA0_SP         0x20
#define V4KIP_SIGMA0_IP         0x24
#define V4KIP_SIGMA0_LOW        0x28
#define V4KIP_SIGMA0_HIGH       0x2C
#define V4KIP_ROOT_SP           0x40
#define V4KIP_ROOT_IP           0x44
#define V4KIP_ROOT_LOW          0x48
#define V4KIP_ROOT_HIGH         0x4C
#define V4KIP_MEM_INFO		0x54
#define V4KIP_BOOT_INFO		0xB8
#elif L4_MWORD_BITS == 64
#define V4KIP_SIGMA0_SP         0x40
#define V4KIP_SIGMA0_IP         0x48
#define V4KIP_SIGMA0_LOW        0x50
#define V4KIP_SIGMA0_HIGH       0x58
#define V4KIP_ROOT_SP           0x80
#define V4KIP_ROOT_IP           0x88
#define V4KIP_ROOT_LOW          0x90
#define V4KIP_ROOT_HIGH         0x98
#define V4KIP_MEM_INFO		0xA8
#define V4KIP_BOOT_INFO		0x170
#else
#error unknown architecture
#endif

#define V4KIP_WORD(kip,offset)	((l4_umword_t*)(((char*)(kip))+(offset)))

/**
 * @brief Initialize KIP prototype for Fiasco/v4.
 */
void
init_kip_v4 (void *l4i, boot_info_t *bi, l4util_mb_info_t *mbi, 
    Region_list *ram, Region_list *regions)
{
  Mem_desc *md = Mem_desc::first(l4i);
  for (Region const* c = ram->begin(); c != ram->end(); ++c)
    (md++)->set(c->begin(), c->end(), Mem_desc::Conventional);

#if 0
  // 1: all memory is accessible for users
  *p++ = 0 | 4;
  *p++ = ~0UL;
  num_info++;

  // 4: (additional) 20MB kernel memory
  if (mem_end > (240 << 20))
      mem_end = 240 << 20;
  *p++ = (mem_end - (20 << 20)) | 2;
  *p++ = mem_end;
  num_info++;
#endif

  // VGA
  (md++)->set(0xA0000, 0xBFFFF, Mem_desc::Shared);
  // 6: BIOS
  (md++)->set(0xC0000, 0xEFFFF, Mem_desc::Shared);

  unsigned long s0_low = ~0UL, s0_high = 0;
  unsigned long root_low = ~0UL, root_high = 0;
  
  for (Region const *c = regions->begin(); c != regions->end(); ++c)
    {
      Mem_desc::Mem_type type = Mem_desc::Reserved;
      unsigned char sub_type = 0;
      switch (c->type())
	{
	case Region::No_mem:
	case Region::Ram:
	case Region::Boot:
	  continue;
	case Region::Kernel:
	  type = Mem_desc::Reserved;
	  break;
	case Region::Sigma0:
	  type = Mem_desc::Dedicated;
	  if (s0_low > c->begin())
	    s0_low = c->begin();
	  if (s0_high < c->end())
	    s0_high = c->end();
	  break;
	case Region::Root:
	  type = Mem_desc::Bootloader;
	  if (root_low > c->begin())
	    root_low = c->begin();
	  if (root_high < c->end())
	    root_high = c->end();
	  break;
	case Region::Arch:
	  type = Mem_desc::Arch;
	  sub_type = c->sub_type();
	  break;
	}
      (md++)->set(c->begin(), c->end() - 1, type, sub_type);
    }

  *V4KIP_WORD(l4i,V4KIP_SIGMA0_LOW)  = s0_low;
  *V4KIP_WORD(l4i,V4KIP_SIGMA0_HIGH) = s0_high+1;
  *V4KIP_WORD(l4i,V4KIP_SIGMA0_IP)   = bi->sigma0_start;

  *V4KIP_WORD(l4i,V4KIP_ROOT_LOW)  = root_low;
  *V4KIP_WORD(l4i,V4KIP_ROOT_HIGH) = root_high+1;
  *V4KIP_WORD(l4i,V4KIP_ROOT_IP)   = bi->roottask_start;
  *V4KIP_WORD(l4i,V4KIP_ROOT_SP)   = bi->roottask_stack;

  *V4KIP_WORD(l4i,V4KIP_BOOT_INFO) = (unsigned long)mbi;

  // set the mem_info variable: count of mem descriptors
  Mem_desc::count(l4i, md - Mem_desc::first(l4i));
}
