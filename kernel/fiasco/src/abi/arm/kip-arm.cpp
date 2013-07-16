/*
 * ARM Kernel-Info Page
 */

INTERFACE [arm]:

#include "types.h"

EXTENSION class Kip
{
public:
  struct Platform_info
  {
    char name[16];
    Unsigned32 is_mp;
    struct
    {
      struct
      {
        Unsigned32 MIDR, CTR, ID_MMFR0, ID_ISAR0;
      } cpuinfo;
    } arch;
    Unsigned32 pad[3];
  };

  /* 0x00 */
  Mword      magic;
  Mword      version;
  Unsigned8  offset_version_strings;
  Unsigned8  fill0[3];
  Unsigned8  kip_sys_calls;
  Unsigned8  fill1[3];

  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */

  /* 0x10 */
  Mword      sched_granularity;
  Mword      _res1[3];

  /* 0x20 */
  Mword      sigma0_sp, sigma0_ip;
  Mword      _res2[2];

  /* 0x30 */
  Mword      sigma1_sp, sigma1_ip;
  Mword      _res3[2];

  /* 0x40 */
  Mword      root_sp, root_ip;
  Mword      _res4[2];

  /* 0x50 */
  Mword      _res_50;
  Mword      _mem_info;
  Mword      _res_58[2];

  /* 0x60 */
  Mword      _res5[16];

  /* 0xA0 */
  volatile Cpu_time clock;
  Unsigned64 _res6;

  /* 0xB0 */
  Mword      frequency_cpu;
  Mword      frequency_bus;

  /* 0xB8 */
  Mword      _res7[10];

  /* 0xE0 */
  Mword      user_ptr;
  Mword      vhw_offset;
  Unsigned32 _res8[2];

  /* 0xF0 */
  Platform_info platform_info;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && debug]:

IMPLEMENT inline
void
Kip::debug_print_syscalls() const
{}
