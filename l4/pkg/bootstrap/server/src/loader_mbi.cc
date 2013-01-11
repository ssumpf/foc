/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
/*
 * Used when our boot loader does not supply a mbi structure.
 */

#include <stdio.h>

#include "loader_mbi.h"

l4util_mb_info_t loader_mbi;

l4util_mb_info_t *init_loader_mbi(void *realmode_pointer)
{
  loader_mbi.flags      = L4UTIL_MB_MEMORY;
  loader_mbi.mem_lower  = 0x9f * 4;

  unsigned long *cmd_line_ptr;

  loader_mbi.mem_upper  = *(unsigned long *)((char *)realmode_pointer + 0x1e0);
  printf("Detected memory size: %dKB\n", loader_mbi.mem_upper);

  cmd_line_ptr = (unsigned long *)((char *)realmode_pointer + 0x228);
  if (cmd_line_ptr && *cmd_line_ptr)
    {
      loader_mbi.flags |= L4UTIL_MB_CMDLINE;
      loader_mbi.cmdline = *cmd_line_ptr;
    }
  
  return &loader_mbi;
}
