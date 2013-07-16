/*!
 * \file
 * \brief  Support for Tegra 3 platforms
 *
 * \date   2013
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/*
 * (c) 2013 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "support.h"
#include <l4/drivers/uart_pxa.h>

namespace {
class Platform_arm_tegra3 : public Platform_single_region_ram
{
  bool probe() { return true; }

  void init()
  {
    static L4::Uart_16550 _uart(25459200);
    unsigned long uart_addr;
    switch (2) {
      case 0: uart_addr = 0x70006000;
      case 2: uart_addr = 0x70006200;
    };
    static L4::Io_register_block_mmio r(uart_addr, 2);
    _uart.startup(&r);
    _uart.change_mode(3, 115200);
    set_stdio_uart(&_uart);
  }
};
}

REGISTER_PLATFORM(Platform_arm_tegra3);
