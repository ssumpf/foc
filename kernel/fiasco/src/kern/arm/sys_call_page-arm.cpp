INTERFACE:

#include "types.h"

//----------------------------------------------------------------------------
IMPLEMENTATION[arm && armv5]:

PRIVATE static inline NOEXPORT NEEDS["types.h"]
void
Sys_call_page::set_utcb_get_code(Mword *sys_calls)
{
  *(sys_calls++) = 0xe3e00a02; // mvn r0, #8192
  *(sys_calls++) = 0xe5100fff; // ldr r0, [r0, -#4095]
  *(sys_calls++) = 0xe1a0f00e; // mov pc, lr
}

//----------------------------------------------------------------------------
IMPLEMENTATION[arm && (armv6 || armv7)]:

PRIVATE static inline NOEXPORT NEEDS["types.h"]
void
Sys_call_page::set_utcb_get_code(Mword *sys_calls)
{
  *(sys_calls++) = 0xee1d0f70; // mrc 15, 0, r0, cr13, cr0, {3}
  *(sys_calls++) = 0xe1a0f00e; // mov pc, lr
}

//----------------------------------------------------------------------------
IMPLEMENTATION:

#include <cstring>
#include "kernel_task.h"
#include "mem_layout.h"
#include "vmem_alloc.h"
#include "panic.h"

IMPLEMENT static
void
Sys_call_page::init()
{
  Mword *sys_calls = (Mword*)Mem_layout::Syscalls;
  if (!Vmem_alloc::page_alloc(sys_calls,
                              Vmem_alloc::NO_ZERO_FILL, Vmem_alloc::User))
    panic("FIASCO: can't allocate system-call page.\n");

  for (unsigned i = 0; i < Config::PAGE_SIZE; i += sizeof(Mword))
    *(sys_calls++) = 0xef000000; // svc

  set_utcb_get_code((Mword*)(Mem_layout::Syscalls + 0xf00));

  Kernel_task::kernel_task()
      ->set_attributes(Mem_layout::Syscalls,
                       Mem_space::Page_cacheable | Mem_space::Page_user_accessible);

  Mem_unit::flush_cache();
}
