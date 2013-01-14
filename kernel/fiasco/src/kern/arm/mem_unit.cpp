INTERFACE [arm]:

#include "kmem.h"
#include "mmu.h"

class Mem_unit : public Mmu< Kmem::Cache_flush_area >
{
public:
  static void tlb_flush();
  static void dtlb_flush( void* va );
  static void dtlb_flush();
  static void tlb_flush(unsigned long asid);
  static void dtlb_flush(unsigned long asid);
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

IMPLEMENT inline
void Mem_unit::tlb_flush()
{
  asm volatile (
      "mcr p15, 0, %0, c8, c7, 0x00 \n"
      :
      : "r" (0)
      : "memory" ); // TLB flush
}


IMPLEMENT inline
void Mem_unit::dtlb_flush( void* va )
{
  asm volatile (
      "mcr p15, 0, %0, c8, c6, 0x01 \n"
      :
      : "r"((unsigned long)va & 0xfffff000)
      : "memory" ); // TLB flush
}

IMPLEMENT inline
void Mem_unit::dtlb_flush()
{
  asm volatile (
      "mcr p15, 0, %0, c8, c6, 0x0 \n"
      :
      : "r"(0)
      : "memory" ); // TLB flush
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv5]:

PUBLIC static inline
void Mem_unit::tlb_flush( void* va, unsigned long)
{
  asm volatile (
      "mcr p15, 0, %0, c8, c7, 0x01 \n"
      :
      : "r"((unsigned long)va & 0xfffff000)
      : "memory" ); // TLB flush
}


IMPLEMENT inline
void Mem_unit::tlb_flush(unsigned long)
{
  asm volatile (
      "mcr p15, 0, r0, c8, c7, 0x00 \n"
      :
      :
      : "memory" ); // TLB flush
}

IMPLEMENT inline
void Mem_unit::dtlb_flush(unsigned long)
{
  asm volatile (
      "mcr p15, 0, %0, c8, c6, 0x0 \n"
      :
      : "r"(0)
      : "memory" ); // TLB flush
}


//---------------------------------------------------------------------------
IMPLEMENTATION [arm && (armv6 || armv7)]:

PUBLIC static inline
void Mem_unit::tlb_flush(void *va, unsigned long asid)
{
  if (asid == ~0UL)
    return;
  btc_flush();
  asm volatile (
      "mcr p15, 0, %1, c7, c10, 4   \n" // drain write buffer
      "mcr p15, 0, %0, c8, c7, 1    \n" // flush both TLB entry
      :
      : "r"(((unsigned long)va & 0xfffff000) | (asid & 0xff)),
        "r" (0)
      : "memory" );
}

IMPLEMENT inline
void Mem_unit::tlb_flush(unsigned long asid)
{
  btc_flush();
  asm volatile (
      "mcr p15, 0, %1, c7, c10, 4   \n" // drain write buffer
      "mcr p15, 0, %0, c8, c7, 2    \n" // flush both TLB with asid
      :
      : "r"(asid),
        "r" (0)
      : "memory" );
}

IMPLEMENT inline
void Mem_unit::dtlb_flush(unsigned long asid)
{
  asm volatile (
      "mcr p15, 0, r0, c7, c10, 4   \n" // drain write buffer
      "mcr p15, 0, %0, c8, c6, 2    \n" // flush data TLB with asid
      :
      : "r"(asid)
      : "memory" );
}

