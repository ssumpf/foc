/**
 * \file   arnadale.cc
 * \brief  Support for the OpenMoko platform
 *
 * \date   2012
 * \author Genode Labs
 *
 */


#include "support.h"

#include <l4/drivers/uart_s3c2410.h>
#include <l4/drivers/uart_dummy.h>

namespace {
class Platform_arm_arndale : public Platform_single_region_ram
{
  bool probe() { return true; }

  void init()
  {
    static L4::Uart_s5pv210 _uart;
    static L4::Io_register_block_mmio r(0x12C20000);

    _uart.startup(&r);
    set_stdio_uart(&_uart);
  }
};
}

REGISTER_PLATFORM(Platform_arm_arndale);
