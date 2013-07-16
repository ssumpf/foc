INTERFACE [arm]:

#include <cstddef>
#include "types.h"
#include "mem_layout.h"

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "cpu.h"

extern char kernel_page_directory[];

namespace Bootstrap {

struct Order_t;
struct Phys_addr_t;
struct Virt_addr_t;

typedef cxx::int_type<unsigned, Order_t> Order;
typedef cxx::int_type_order<Unsigned32, Virt_addr_t, Order> Virt_addr;

enum
{
  Virt_ofs = Mem_layout::Sdram_phys_base - Mem_layout::Map_base,
};

}

//---------------------------------------------------------------------------
INTERFACE [arm && armv5]:

namespace Bootstrap {
inline void set_asid()
{}
}

//---------------------------------------------------------------------------
INTERFACE [arm && (armv6 || armv7)]:

namespace Bootstrap {
inline void
set_asid()
{
  asm volatile ("mcr p15, 0, %0, c13, c0, 1" : : "r" (0)); // ASID 0
}
}

//---------------------------------------------------------------------------
INTERFACE [arm && !arm_lpae]:

namespace Bootstrap {
inline void
set_ttbr(Mword pdir)
{
  asm volatile("mcr p15, 0, %[pdir], c2, c0" // TTBR0
               : : [pdir] "r" (pdir));
}
}

//---------------------------------------------------------------------------
INTERFACE [arm && arm_lpae]:

namespace Bootstrap {
inline void
set_ttbr(Mword pdir)
{
  asm volatile("mcrr p15, 0, %[pdir], %[null], c2" // TTBR0
               : :
               [pdir]  "r" (pdir),
               [null]  "r" (0));
}
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && arm1176_cache_alias_fix]:

namespace Bootstrap {
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
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && !arm1176_cache_alias_fix]:

namespace Bootstrap {
static void do_arm_1176_cache_alias_workaround() {}
}


//---------------------------------------------------------------------------
IMPLEMENTATION [arm && arm_lpae]:

#include <cxx/cxx_int>

extern char kernel_lpae_dir[];

namespace Bootstrap {
typedef cxx::int_type_order<Unsigned64, Phys_addr_t, Order> Phys_addr;
inline Order map_page_order() { return Order(21); }

inline Phys_addr pt_entry(Phys_addr pa, bool cache, bool local)
{
  Phys_addr res = cxx::mask_lsb(pa, map_page_order()) | Phys_addr(1); // this is a block

  if (local)
    res |= Phys_addr(1 << 11); // nG flag

  if (cache)
    res |= Phys_addr(8);

  res |= Phys_addr(1 << 10); // AF
  res |= Phys_addr(3 << 8);  // Inner sharable
  return res;
}

inline Phys_addr init_paging(void *const page_dir)
{
  Phys_addr *const lpae = reinterpret_cast<Phys_addr*>(kernel_lpae_dir + Virt_ofs);

  for (unsigned i = 0; i < 4; ++i)
    lpae[i] = Phys_addr(((Address)page_dir + 0x1000 * i) | 3);;

  asm volatile ("mcr p15, 0, %0, c10, c2, 0 \n" // MAIR0
                : : "r"(Page::Mair0_bits));

  return Phys_addr((Mword)lpae);
}

};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && !arm_lpae]:

#include <cxx/cxx_int>

namespace Bootstrap {
typedef cxx::int_type_order<Unsigned32, Phys_addr_t, Order> Phys_addr;
inline Order map_page_order() { return Order(20); }

inline Phys_addr pt_entry(Phys_addr pa, bool cache, bool local)
{
  return cxx::mask_lsb(pa, map_page_order())
                | Phys_addr(cache ? Page::Section_cachable : Page::Section_no_cache)
                | Phys_addr(local ? Page::Section_local : Page::Section_global);
}

inline Phys_addr init_paging(void *const page_dir)
{
  return Phys_addr((Mword)page_dir);
}

};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "kmem_space.h"


namespace Bootstrap {


inline Phys_addr map_page_size_phys() { return Phys_addr(1) << map_page_order(); }
inline Virt_addr map_page_size() { return Virt_addr(1) << map_page_order(); }

static void
map_memory(void volatile *pd, Virt_addr va, Phys_addr pa,
           bool cache, bool local)
{
  Phys_addr *const p = (Phys_addr*)pd;
  p[cxx::int_value<Virt_addr>(va >> map_page_order())]
    = pt_entry(pa, cache, local);
}

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

extern "C" void _start_kernel(void) __attribute__((long_call));

extern "C" void bootstrap_main()
{
  typedef Bootstrap::Phys_addr Phys_addr;
  typedef Bootstrap::Virt_addr Virt_addr;
  typedef Bootstrap::Order Order;

  void *const page_dir = kernel_page_directory + Bootstrap::Virt_ofs;

  Unsigned32 tbbr = cxx::int_value<Phys_addr>(Bootstrap::init_paging(page_dir))
                    | Page::Ttbr_bits;

  Virt_addr va;
  Phys_addr pa;
  // map sdram linear from 0xf0000000
  for (va = Virt_addr(Mem_layout::Map_base), pa = Phys_addr(Mem_layout::Sdram_phys_base);
       va < Virt_addr(Mem_layout::Map_base + (4 << 20));
       va += Bootstrap::map_page_size(), pa += Bootstrap::map_page_size_phys())
    Bootstrap::map_memory(page_dir, va, pa, true, false);

  // map sdram 1:1
  for (va = Virt_addr(Mem_layout::Sdram_phys_base);
       va < Virt_addr(Mem_layout::Sdram_phys_base + (4 << 20));
       va += Bootstrap::map_page_size())
    Bootstrap::map_memory(page_dir, va, Phys_addr(cxx::int_value<Virt_addr>(va)), true, true);

  unsigned domains = 0x55555555; // client for all domains
  unsigned control = Config::Cache_enabled
                     ? Cpu::Cp15_c1_cache_enabled : Cpu::Cp15_c1_cache_disabled;

  Mmu<Bootstrap::Cache_flush_area, true>::flush_cache();

  Bootstrap::do_arm_1176_cache_alias_workaround();
  Bootstrap::set_asid();

  asm volatile("mcr p15, 0, %[ttbcr], c2, c0, 2" // TTBCR
               : : [ttbcr] "r" (Page::Ttbcr_bits));
  Mem::dsb();
  asm volatile("mcr p15, 0, %[null], c8, c7, 0" // TLBIALL
               : : [null]  "r" (0));
  Mem::dsb();
  asm volatile("mcr p15, 0, %[doms], c3, c0" // domains
               : : [doms]  "r" (domains));

  Bootstrap::set_ttbr(tbbr | Page::Ttbr_bits);

  asm volatile("mcr p15, 0, %[control], c1, c0" // control
               : : [control] "r" (control));
  Mem::isb();

  // The first 4MB of phys memory are always mapped to Map_base
  Mem_layout::add_pmem(Mem_layout::Sdram_phys_base, Mem_layout::Map_base,
                       4 << 20);

  _start_kernel();

  while(1)
    ;
}
