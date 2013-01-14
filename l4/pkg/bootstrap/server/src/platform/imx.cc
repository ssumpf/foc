/*!
 * \file   support_imx.cc
 * \brief  Support for the i.MX platform
 *
 * \date   2008-02-04
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

#include "support.h"
#include <l4/drivers/uart_imx.h>

namespace {
class Platform_arm_imx : public Platform_single_region_ram
{
  bool probe() { return true; }

  void init()
  {
#ifdef PLATFORM_TYPE_imx21
    static L4::Io_register_block_mmio r(0x1000A000);
    static L4::Uart_imx21 _uart;
#elif defined(PLATFORM_TYPE_imx35)
    static L4::Uart_imx35 _uart;
    static L4::Io_register_block_mmio r(0x43f90000); // UART-1
    //static L4::Io_register_block_mmio r(0x43f94000); // UART-2
    //static L4::Io_register_block_mmio r(0x5000c000); // UART-3
#elif defined(PLATFORM_TYPE_imx51)
    static L4::Io_register_block_mmio r(0x73fbc000);
    static L4::Uart_imx51 _uart;
#elif defined(PLATFORM_TYPE_imx6)
    //static L4::Io_register_block_mmio r(0x02020000); // UART1
    static L4::Io_register_block_mmio r(0x021e8000); // UART2
    static L4::Uart_imx6 _uart;
#else
#error Which platform type?
#endif
    _uart.startup(&r);
    set_stdio_uart(&_uart);
  }
};
}

REGISTER_PLATFORM(Platform_arm_imx);
