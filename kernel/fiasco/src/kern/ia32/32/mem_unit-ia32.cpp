INTERFACE[ia32]:

#include "types.h"

class Mem_unit
{
};


IMPLEMENTATION[ia32]:

/** Flush the whole TLB.
 */
PUBLIC static inline
void
Mem_unit::tlb_flush()
{
  unsigned dummy;
  asm volatile ("mov %%cr3,%0; mov %0,%%cr3 " : "=r"(dummy));
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
