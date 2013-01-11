/**
 * \file
 * \brief  Support for the MPC52000
 *
 * \date   2009-02-16
 * \author Sebastian Sumpf <sumpf@os.inf.tu-dresden.de>
 *
 */
/*
 * (c) 2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "support.h"
#include <l4/drivers/uart_of.h>


namespace {
class Platform_ppc_mpc52000 : public Platform_base
{
  bool probe() { return true; }

  void init()
  {
    static L4::Uart_of _uart;
    static L4::Io_register_block_mmio r(0);
    _uart.startup(&r);
    set_stdio_uart(&_uart);
  }
  void setup_memory_map(l4util_mb_info_t *,
                        Region_list *, Region_list *)
  {
    // still done in startup.cc
  }
};
}

REGISTER_PLATFORM(Platform_ppc_mpc52000);
