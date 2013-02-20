/*!
 * \file   support.h
 * \brief  Support header file
 *
 * \date   2008-01-02
 * \author Adam Lackorznynski <adam@os.inf.tu-dresden.de>
 *
 */
/*
 * (c) 2008-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#ifndef __BOOTSTRAP__SUPPORT_H__
#define __BOOTSTRAP__SUPPORT_H__

#include <l4/drivers/uart_base.h>
#include <l4/util/mb_info.h>
#include <stdio.h>
#include "region.h"

L4::Uart *uart();
void set_stdio_uart(L4::Uart *uart);

void platform_init();
unsigned long scan_ram_size(unsigned long base_addr, unsigned long max_scan_size_mb);


class Platform_base
{
public:
  virtual void init() = 0;
  virtual void setup_memory_map(l4util_mb_info_t *mbi,
                                Region_list *ram, Region_list *regions) = 0;
  virtual bool probe() = 0;

  virtual void reboot()
  {
    while (1)
      ;
  }

  // remember the chosen platform
  static Platform_base *platform;

  // find a platform
  static void iterate_platforms()
  {
    extern Platform_base *__PLATFORMS_BEGIN[];
    extern Platform_base *__PLATFORMS_END[];
    for (Platform_base **p = __PLATFORMS_BEGIN; p < __PLATFORMS_END; ++p)
      if (*p && (*p)->probe())
        {
          platform = *p;
          platform->init();
          break;
        }
  }
};

#define REGISTER_PLATFORM(type) \
      static type type##_inst; \
      static type * const __attribute__((section(".platformdata"),used)) type##_inst_p = &type##_inst

#ifdef RAM_SIZE_MB
class Platform_single_region_ram : public Platform_base
{
public:
  void setup_memory_map(l4util_mb_info_t *,
                        Region_list *ram, Region_list *)
  {
    unsigned long ram_size_mb = scan_ram_size(RAM_BASE, RAM_SIZE_MB);
    printf("  Memory size is %ldMB%s (%08lx - %08lx)\n",
           ram_size_mb, ram_size_mb != RAM_SIZE_MB ? " (Limited by Scan)" : "",
           (unsigned long)RAM_BASE, RAM_BASE + (ram_size_mb << 20));
    ram->add(Region::n(RAM_BASE,
                       (unsigned long long)RAM_BASE + (ram_size_mb << 20),
                       ".ram", Region::Ram));
  }
};
#endif

#endif /* __BOOTSTRAP__SUPPORT_H__ */
