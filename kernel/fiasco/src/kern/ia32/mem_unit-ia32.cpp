INTERFACE[ia32 || amd64]:

#include "types.h"

class Mem_unit
{
};


IMPLEMENTATION[ia32 || amd64]:

/** Flush the whole TLB.
 */
PUBLIC static inline ALWAYS_INLINE
void
Mem_unit::tlb_flush()
{
  Mword dummy;
  __asm__ __volatile__ ("mov %%cr3,%0; mov %0,%%cr3 " : "=r"(dummy) : : "memory");
}


/** Flush TLB at virtual address.
 */
PUBLIC static inline ALWAYS_INLINE
void
Mem_unit::tlb_flush(Address addr)
{
  __asm__ __volatile__ ("invlpg %0" : : "m" (*(char*)addr) : "memory");
}

PUBLIC static inline ALWAYS_INLINE
void
Mem_unit::clean_dcache(void *)
{}
