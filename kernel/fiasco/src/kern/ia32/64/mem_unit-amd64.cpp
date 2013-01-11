INTERFACE[amd64]:

#include "types.h"

class Mem_unit
{
};


IMPLEMENTATION[amd64]:

/** Flush the whole TLB.
 */
PUBLIC static inline
void
Mem_unit::tlb_flush()
{
  Unsigned64 dummy;
  asm volatile ("movq %%cr3,%0; movq %0,%%cr3 " : "=r"(dummy));
}


/** Flush TLB at virtual address.
 */
PUBLIC static inline
void
Mem_unit::tlb_flush(Address addr)
{
  asm volatile ("invlpg %0" : : "m" (*(char*)addr) : "memory");
}

PUBLIC static inline
void
Mem_unit::clean_dcache(void *)
{}
