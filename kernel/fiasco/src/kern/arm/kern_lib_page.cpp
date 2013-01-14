INTERFACE [arm]:

class Kern_lib_page
{
public:
  static void init();
};


//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <panic.h>

#include "kmem_alloc.h"
#include "kmem_space.h"
#include "pagetable.h"
#include "ram_quota.h"

IMPLEMENT
void Kern_lib_page::init()
{
  extern char kern_lib_start;
  Pte pte = Kmem_space::kdir()->walk((void *)Kmem_space::Kern_lib_base,
                                     Config::PAGE_SIZE, true,
                                     Kmem_alloc::q_allocator(Ram_quota::root),
                                     Kmem_space::kdir());

  if (pte.lvl() == 0) // allocation of second level faild
    panic("FATAL: Error mapping kernel-lib page to %p\n",
          (void *)Kmem_space::Kern_lib_base);

  pte.set((Address)&kern_lib_start - Mem_layout::Map_base
          + Mem_layout::Sdram_phys_base,
          Config::PAGE_SIZE, Mem_page_attr(Page::USER_RO | Page::CACHEABLE),
          true);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && !armv6plus]:

asm (
    ".p2align(12)                        \n"
    "kern_lib_start:                     \n"

    // atomic add
    // r0: memory reference
    // r1: delta value
    "  ldr r2, [r0]			 \n"
    "  add r2, r2, r1                    \n"
    "  nop                               \n"
    "  str r2, [r0]                      \n"
    // forward point
    "  mov r0, r2                        \n"
    "  mov pc, lr                        \n"
    // return: always succeeds, new value

    // compare exchange
    // r0: memory reference
    // r1: cmp value
    // r2: new value
    // r3: tmp
    ".p2align(8)                         \n"
    "  ldr r3, [r0]			 \n"
    "  cmp r3, r1                        \n"
    "  nop                               \n"
    "  streq r2, [r0]                    \n"
    // forward point
    "  moveq r0, #1                      \n"
    "  movne r0, #0                      \n"
    "  mov pc, lr                        \n"
    // return result: 1 success, 0 failure

    // exchange
    //  in-r0: memory reference
    //  in-r1: new value
    // out-r0: old value
    // tmp-r2
    ".p2align(8)                         \n"
    "  ldr r2, [r0]			 \n"
    "  nop                               \n"
    "  nop                               \n"
    "  str r1, [r0]                      \n"
    // forward point
    "  mov r0, r2                        \n"
    "  mov pc, lr                        \n"
    // return: always succeeds, old value
    );

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv6plus]:

asm (
    ".p2align(12)                        \n"
    "kern_lib_start:                     \n"

    // no restart through kernel entry code

    // atomic add
    // r0: memory reference
    // r1: delta value
    // r2: temp register
    // r3: temp register
    " 1:                                 \n"
    " ldrex r2, [r0]                     \n"
    " add   r2, r2, r1                   \n"
    " strex r3, r2, [r0]                 \n"
    " teq r3, #0                         \n"
    " bne 1b                             \n"
    " mov r0, r2                         \n"
    " mov pc, lr                         \n"
    // return: always succeeds, new value


    // compare exchange
    // r0: memory reference
    // r1: cmp value
    // r2: new value
    // r3: tmp reg
    ".p2align(8)                         \n"
    "  1: ldrex r3, [r0]		 \n"
    "  cmp r3, r1                        \n"
    "  movne r0, #0                      \n"
    "  movne pc, lr                      \n"
    "  strex r3, r2, [r0]                \n"
    "  teq r3, #0                        \n"
    "  bne 1b                            \n"
    "  mov r0, #1                        \n"
    "  mov pc, lr                        \n"
    // return result: 1 success, 0 failure


    // exchange
    //  in-r0: memory reference
    //  in-r1: new value
    // out-r0: old value
    ".p2align(8)                         \n"
    "  1:                                \n"
    "  ldrex r2, [r0]			 \n"
    "  strex r3, r1, [r0]                \n"
    "  cmp r3, #0                        \n"
    "  bne 1b                            \n"
    "  mov r0, r2                        \n"
    "  mov pc, lr                        \n"
    // return: always succeeds, old value
    );
