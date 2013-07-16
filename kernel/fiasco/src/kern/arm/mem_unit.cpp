INTERFACE [arm]:

#include "mem_layout.h"
#include "mmu.h"

class Mem_unit : public Mmu< Mem_layout::Cache_flush_area >
{
public:
  enum : Mword
  {
    Asid_kernel  = 0UL,
    Asid_invalid = ~0UL
  };

  static void tlb_flush();
  static void dtlb_flush(void *va);
  static void tlb_flush(unsigned long asid);
  static void tlb_flush(void *va, unsigned long asid);
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

IMPLEMENT inline
void Mem_unit::tlb_flush()
{
  asm volatile("mcr p15, 0, %0, c8, c7, 0" // TLBIALL
               : : "r" (0) : "memory");
}

IMPLEMENT inline
void Mem_unit::dtlb_flush(void *va)
{
  asm volatile("mcr p15, 0, %0, c8, c6, 1" // DTLBIMVA
               : : "r" ((unsigned long)va & 0xfffff000) : "memory");
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv5]:

IMPLEMENT inline
void Mem_unit::tlb_flush(void *va, unsigned long)
{
  asm volatile("mcr p15, 0, %0, c8, c7, 1"
               : : "r" ((unsigned long)va & 0xfffff000) : "memory");
}

IMPLEMENT inline
void Mem_unit::tlb_flush(unsigned long)
{
  asm volatile("mcr p15, 0, r0, c8, c7, 0" : : "r" (0) : "memory");
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && (armv6 || armv7)]:

IMPLEMENT inline
void Mem_unit::tlb_flush(void *va, unsigned long asid)
{
  if (asid == Asid_invalid)
    return;
  btc_flush();
  Mem::dsb();
  asm volatile("mcr p15, 0, %0, c8, c7, 1" // TLBIMVA
               : : "r" (((unsigned long)va & 0xfffff000) | asid) : "memory");
}

IMPLEMENT inline
void Mem_unit::tlb_flush(unsigned long asid)
{
  btc_flush();
  Mem::dsb();
  asm volatile("mcr p15, 0, %0, c8, c7, 2" // TLBIASID
               : : "r" (asid) : "memory");
}
