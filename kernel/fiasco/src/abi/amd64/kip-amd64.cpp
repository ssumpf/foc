/*
 * AMD64 Kernel-Info Page
 */

INTERFACE [amd64]:

#include "types.h"

EXTENSION class Kip
{
public:
  struct Platform_info
  {
    char name[16];
    Unsigned32 is_mp;
  };

  /* 0x00 */
  Mword      magic;
  Mword      version;
  Unsigned8  offset_version_strings;
  Unsigned8  fill2[7];
  Unsigned8  kip_sys_calls;
  Unsigned8  fill3[7];

  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */

  /* 0x20 */
  Mword      sched_granularity;
  Mword      _res1[3];

  /* 0x40 */
  Mword      sigma0_sp, sigma0_ip;
  Mword      _res2[2];

  /* 0x60 */
  Mword      sigma1_sp, sigma1_ip;
  Mword      _res3[2];

  /* 0x80 */
  Mword      root_sp, root_ip;
  Mword      _res4[2];

  /* 0xA0 */
  Mword      _res_a0;
  Mword      _mem_info;
  Mword      _res_b0[2];

  /* 0xC0 */
  Mword      _res5[16];

  /* 0x140 */
  volatile Cpu_time clock;
  Unsigned64 _res6;

  /* 0x150 */
  Mword      frequency_cpu;
  Mword      frequency_bus;

  /* 0x160 */
  Mword      _res7[12];

  /* 0x1C0 */
  Mword      user_ptr;
  Mword      vhw_offset;

  /* 0x1D0 */
  Mword      _res8[2];

  /* 0x1E0 */
  Platform_info platform_info;
  Unsigned32 __reserved[3];
};

