/*!
 * \file   support_pxa.cc
 * \brief  Support for the PXA platform
 *
 * \date   2008-01-04
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
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

#include <l4/drivers/uart_pxa.h>
#include "support.h"

namespace {
class Platform_arm_pxa : public Platform_single_region_ram
{
  bool probe() { return true; }

  void init()
  {
    static L4::Uart_16550 _uart(L4::Uart_16550::Base_rate_pxa);
    static L4::Io_register_block_mmio regs(0x40100000, 2);
    _uart.startup(&regs);
    set_stdio_uart(&_uart);
  }
};
}

REGISTER_PLATFORM(Platform_arm_pxa);
