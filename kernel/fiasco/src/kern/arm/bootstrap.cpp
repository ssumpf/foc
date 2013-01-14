INTERFACE [arm]:

#include <cstddef>
#include "types.h"
#include "mem_layout.h"

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "cpu.h"

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv5]:

enum
{
  Section_cachable = 0x40e,
  Section_no_cache = 0x402,
  Section_local    = 0,
  Section_global   = 0,
};

void
set_asid()
{}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv6plus && (mpcore || armca9)]:

enum
{
  Section_shared = 1UL << 16,
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv6plus && !(mpcore || armca9)]:

enum
{
  Section_shared = 0,
};


//---------------------------------------------------------------------------
IMPLEMENTATION [arm && (armv6 || armv7)]:

enum
{
  Section_cachable = 0x5406 | Section_shared,
  Section_no_cache = 0x0402 | Section_shared,
  Section_local    = (1 << 17),
  Section_global   = 0,
};

void
set_asid()
{
  asm volatile ("MCR p15, 0, %0, c13, c0, 1" : : "r" (0)); // ASID 0
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && arm1176_cache_alias_fix]:

static void
do_arm_1176_cache_alias_workaround()
{
  Mword v;
  asm volatile ("mrc p15, 0, %0, c0, c0, 1   \n" : "=r" (v));
  if (v & ((1 << 23) | (1 << 11)))
    { // P bits set
      asm volatile ("mrc p15, 0, r0, c1, c0, 1   \n"
                    "orr r0, r0, #(1 << 6)       \n"
                    "mcr p15, 0, r0, c1, c0, 1   \n"
                    : : : "r0");
    }
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && !arm1176_cache_alias_fix]:

static void do_arm_1176_cache_alias_workaround() {}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "kmem_space.h"
#include "pagetable.h"

void
map_1mb(void *pd, Address va, Address pa, bool cache, bool local)
{
  Unsigned32 *const p = (Unsigned32*)pd;
  p[va >> 20] = (pa & 0xfff00000)
                | (cache ? Section_cachable : Section_no_cache)
                | (local ? Section_local : Section_global);
}

// This is a template so that we can have the static_assertion, checking the
// right value at compile time. At runtime we probably won't see anything
// as this also affects the UART mapping.
template< Address PA >
static void inline
map_dev(void *pd, unsigned va_slotnr)
{
  static_assert(PA == Invalid_address || (PA & ~0xfff00000) == 0, "Physical address must be 2^20 aligned");
  if (PA != Invalid_address)
    map_1mb(pd, Mem_layout::Registers_map_start + va_slotnr * 0x100000, PA,
            false, false);
}

asm
(
".section .text.init,#alloc,#execinstr \n"
".global _start                        \n"
"_start:                               \n"
"     ldr sp, __init_data              \n"
"     bl	bootstrap_main         \n"

"__init_data:                          \n"
".long _stack                          \n"
".previous                             \n"
".section .bss                         \n"
"	.space	2048                   \n"
"_stack:                               \n"
".previous                             \n"
);


#include "mmu.h"

#include "globalconfig.h"

extern char bootstrap_bss_start[];
extern char bootstrap_bss_end[];
extern char __bss_start[];
extern char __bss_end[];

enum
{
  Virt_ofs = Mem_layout::Sdram_phys_base - Mem_layout::Map_base,
};

extern "C" void bootstrap_main()
{
  extern char kernel_page_directory[];
  void *const page_dir = kernel_page_directory + Virt_ofs;

  Address va, pa;
  // map sdram linear from 0xf0000000
  for (va = Mem_layout::Map_base, pa = Mem_layout::Sdram_phys_base;
       va < Mem_layout::Map_base + (4 << 20); va += 0x100000, pa += 0x100000)
    map_1mb(page_dir, va, pa, true, false);

  // map sdram 1:1
  for (va = Mem_layout::Sdram_phys_base;
       va < Mem_layout::Sdram_phys_base + (4 << 20); va += 0x100000)
    map_1mb(page_dir, va, va, true, true);

  map_hw(page_dir);

  unsigned domains      = 0x55555555; // client for all domains
  unsigned control      = Config::Cache_enabled
                          ? Cpu::Cp15_c1_cache_enabled : Cpu::Cp15_c1_cache_disabled;

  Mmu<Cache_flush_area, true>::flush_cache();

  extern char _start_kernel[];

  do_arm_1176_cache_alias_workaround();
  set_asid();

  asm volatile (
      "mcr p15, 0, %[null], c7, c10, 4\n" // dsb
      "mcr p15, 0, %[null], c8, c7, 0 \n" // tlb flush
      "mcr p15, 0, %[null], c7, c10, 4\n" // dsb
      "mcr p15, 0, %[doms], c3, c0    \n" // domains
      "mcr p15, 0, %[pdir], c2, c0    \n" // pdbr
      "mcr p15, 0, %[control], c1, c0 \n" // control

      "mrc p15, 0, r0, c2, c0, 0      \n" // arbitrary read of cp15
      "mov r0, r0                     \n" // wait for result
      "sub pc, pc, #4                 \n"

      "mov pc, %[start]               \n"
      : :
      [pdir]    "r"((Mword)page_dir | Page_table::Ttbr_bits),
      [doms]    "r"(domains),
      [control] "r"(control),
      [start]   "r"(_start_kernel),
      [null]    "r"(0)
      : "r0"
      );

  while(1)
    ;
}
