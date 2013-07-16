/*!
 * \file   support_x86.cc
 * \brief  Support for the x86 platform
 *
 * \date   2008-01-02
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

#include "support.h"
#include <l4/drivers/uart_pxa.h>
#include <l4/drivers/io_regblock_port.h>

#include <string.h>
#include "base_critical.h"
#include "startup.h"
#include <l4/util/cpu.h>
#include <l4/util/port_io.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>


/** VGA console output */

static void vga_init()
{
  /* Reset any scrolling */
  l4util_out32(0xc, 0x3d4);
  l4util_out32(0, 0x3d5);
  l4util_out32(0xd, 0x3d4);
  l4util_out32(0, 0x3d5);
}

static void vga_putchar(unsigned char c)
{
  static int ofs = -1, esc, esc_val, attr = 0x07;
  unsigned char *vidbase = (unsigned char*)0xb8000;

  base_critical_enter();

  if (ofs < 0)
    {
      /* Called for the first time - initialize.  */
      ofs = 80*2*24;
      vga_putchar('\n');
    }

  switch (esc)
    {
    case 1:
      if (c == '[')
	{
	  esc++;
	  goto done;
	}
      esc = 0;
      break;

    case 2:
      if (c >= '0' && c <= '9')
	{
	  esc_val = 10*esc_val + c - '0';
	  goto done;
	}
      if (c == 'm')
	{
	  attr = esc_val ? 0x0f : 0x07;
	  goto done;
	}
      esc = 0;
      break;
    }

  switch (c)
    {
    case '\n':
      memmove(vidbase, vidbase+80*2, 80*2*24);
      memset(vidbase+80*2*24, 0, 80*2);
      /* fall through... */
    case '\r':
      ofs = 0;
      break;

    case '\t':
      ofs = (ofs + 8) & ~7;
      break;

    case '\033':
      esc = 1;
      esc_val = 0;
      break;

    default:
      /* Wrap if we reach the end of a line.  */
      if (ofs >= 80)
	vga_putchar('\n');

      /* Stuff the character into the video buffer. */
	{
	  volatile unsigned char *p = vidbase + 80*2*24 + ofs*2;
	  p[0] = c;
	  p[1] = attr;
	  ofs++;
	}
      break;
    }

done:
  base_critical_leave();
}

/** Poor man's getchar, only returns raw scan code. We don't need to know
 * _which_ key was pressed, we only want to know _if_ a key was pressed. */
static int raw_keyboard_getscancode(void)
{
  unsigned status, scan_code;

  base_critical_enter();

  l4util_cpu_pause();

  /* Wait until a scan code is ready and read it. */
  status = l4util_in8(0x64);
  if ((status & 0x01) == 0)
    {
      base_critical_leave();
      return -1;
    }
  scan_code = l4util_in8(0x60);

  /* Drop mouse events */
  if ((status & 0x20) != 0)
    {
      base_critical_leave();
      return -1;
    }

  base_critical_leave();
  return scan_code;
}


static inline l4_uint32_t
pci_conf_addr(l4_uint32_t bus, l4_uint32_t dev, l4_uint32_t fn, l4_uint32_t reg)
{ return 0x80000000 | (bus << 16) | (dev << 11) | (fn << 8) | (reg & ~3); }

static l4_uint32_t pci_read(unsigned char bus, l4_uint32_t dev,
                            l4_uint32_t fn, l4_uint32_t reg,
                            unsigned char width)
{
  l4util_out32(pci_conf_addr(bus, dev, fn, reg), 0xcf8);

  switch (width)
    {
    case 8:  return l4util_in8(0xcfc + (reg & 3));
    case 16: return l4util_in16((0xcfc + (reg & 3)) & ~1UL);
    case 32: return l4util_in32(0xcfc);
    }
  return 0;
}

static void pci_write(unsigned char bus, l4_uint32_t dev,
                      l4_uint32_t fn, l4_uint32_t reg,
                      l4_uint32_t val, unsigned char width)
{
  l4util_out32(pci_conf_addr(bus, dev, fn, reg), 0xcf8);

  switch (width)
    {
    case 8:  l4util_out8(val, 0xcfc + (reg & 3)); break;
    case 16: l4util_out16(val, (0xcfc + (reg & 3)) & ~1UL); break;
    case 32: l4util_out32(val, 0xcfc); break;
    }
}

static void pci_enable_io(unsigned char bus, l4_uint32_t dev,
                          l4_uint32_t fn)
{
  unsigned cmd = pci_read(bus, dev, fn, 4, 16);
  pci_write(bus, dev, fn, 4, cmd | 1, 16);
}

namespace {

struct Resource
{
  enum Type { NO_BAR, IO_BAR, MEM_BAR };
  Type type;
  unsigned long base;
  unsigned long len;
  Resource() : type(NO_BAR) {}
};

enum { NUM_BARS = 6 };

struct Serial_board
{
  int num_ports;
  int first_bar;
  bool port_per_bar;
  Resource bars[NUM_BARS];

  unsigned long get_port(int idx)
  {
    if (idx >= num_ports)
      return 0;

    if (port_per_bar)
      return bars[first_bar + idx].base;

    return bars[first_bar].base + 8 * idx;
  }
};


}

static
int pci_handle_serial_dev(unsigned char bus, l4_uint32_t dev,
                          l4_uint32_t subdev, bool print,
                          Serial_board *board)
{
#if 0
  bool dev_enabled = false;
#endif

  // read bars
  int num_iobars = 0;
  int num_membars = 0;
  int first_port = -1;
  for (int bar = 0; bar < NUM_BARS; ++bar)
    {
      int a = 0x10 + bar * 4;

      unsigned v = pci_read(bus, dev, subdev, a, 32);
      pci_write(bus, dev, subdev, a, ~0U, 32);
      unsigned x = pci_read(bus, dev, subdev, a, 32);
      pci_write(bus, dev, subdev, a, v, 32);

      if (!v)
        continue;

      int s;
      for (s = 2; s < 32; ++s)
        if ((x >> s) & 1)
          break;

      board->bars[bar].base = v & ~3UL;
      board->bars[bar].len = 1 << s;
      board->bars[bar].type = (v & 1) ? Resource::IO_BAR : Resource::MEM_BAR;

      if (print)
	printf("BAR%d: %04x (sz=%d)\n", bar, v & ~3, 1 << s);

      switch (board->bars[bar].type)
	{
	case Resource::IO_BAR:
	  ++num_iobars;
	  if (first_port == -1)
	    first_port = bar;
	  break;
	case Resource::MEM_BAR:
	  ++num_membars;
	  break;
	default:
	  break;
	}
    }

  printf("  serial IO card: mem bars: %d   io bars: %d\n", num_membars, num_iobars);

  if (num_membars <= 1 && num_iobars == 1)
    {
      board->first_bar = first_port;
      board->num_ports = board->bars[first_port].len / 8;
      board->port_per_bar = false;
      printf("   use serial IO card: bar=%d ports=%d\n", first_port, board->num_ports);
      pci_enable_io(bus, dev, subdev);
      return 1;
    }


  board->num_ports = 0;
  board->first_bar = -1;

  for (int bar = 0; bar < NUM_BARS; ++bar)
    {
      if (board->bars[bar].type == Resource::IO_BAR && board->bars[bar].len == 8
	  && (board->first_bar == -1
	      || (board->first_bar + board->num_ports) == bar))
	{
	  ++board->num_ports;
	  if (board->first_bar == -1)
	    board->first_bar = bar;
	}
    }

  board->port_per_bar = true;
  return board->num_ports;

#if 0

      // for now we only take IO-BARs of size 8
      if (v & 1)
        {

          if (!scan_only && !dev_enabled)
            {
              pci_enable_io(bus, dev, subdev);
              dev_enabled = true;
            }

          if (scan_only)
            printf("BAR%d: %04x (sz=%d)\n", bar, v & ~3, 1 << s);

          if (s == 3)
            {
              if (scan_only)
                printf("   Potential serial port\n");
              else
                return v & ~3;
            }
        }
      else
        if (scan_only)
          printf("BAR%d: %08x (sz=%d)\n", bar, v & ~0xf, 1 << s);
    }
  return 0;
#endif
}

static unsigned long
_search_pci_serial_devs(Serial_board *board, unsigned look_for_subclass,
                        bool print)
{
  l4_umword_t bus, buses, dev;

  for (bus=0, buses=20; bus<buses; bus++)
    {
      for (dev = 0; dev < 32; dev++)
        {
          unsigned char hdr_type = pci_read(bus, dev, 0, 0xe, 8);
          l4_umword_t subdevs = (hdr_type & 0x80) ? 8 : 1;

          for (l4_umword_t subdev = 0; subdev < subdevs; subdev++)
            {
              unsigned vendor = pci_read(bus, dev, subdev, 0, 16);
              unsigned device = pci_read(bus, dev, subdev, 2, 16);

              if (vendor == 0xffff)
                {
                  if (subdev == 0)
                    break;
                  else
                    continue;
                }

              unsigned classcode = pci_read(bus, dev, subdev, 0x0b, 8);
              unsigned subclass  = pci_read(bus, dev, subdev, 0x0a, 8);

              if (classcode == 0x06 && subclass == 0x04)
                buses++;

              unsigned prog = pci_read(bus, dev, subdev, 9, 8);

              if (print)
                printf("%02lx:%02lx.%1lx Class %02x.%02x Prog %02x: %04x:%04x\n",
                       bus, dev, subdev, classcode, subclass, prog, vendor, device);

              if (classcode == 7 && subclass == look_for_subclass)
                if (unsigned long port = pci_handle_serial_dev(bus, dev,
                                                               subdev, print, board))
                  return port;
            }
        }
    }
  return 0;
}

static unsigned long
search_pci_serial_devs(int port_idx)
{
  Serial_board board;
  if (!_search_pci_serial_devs(&board, 0, true)) // classes should be 7:0
    if (!_search_pci_serial_devs(&board, 0x80, true)) // but sometimes it's 7:80
      return 0;

  return board.get_port(port_idx);
}

class Uart_vga : public L4::Uart
{
public:
  Uart_vga()
  { }

  bool startup(L4::Io_register_block const *)
  {
    vga_init();
    return true;
  }

  ~Uart_vga() {}
  void shutdown() {}
  bool enable_rx_irq(bool) { return false; }
  bool enable_tx_irq(bool) { return false; }
  bool change_mode(Transfer_mode, Baud_rate) { return true; }
  int get_char(bool blocking) const
  {
    int c;
    do
      c = raw_keyboard_getscancode();
    while (blocking && c == -1);
    return c;
  }

  int char_avail() const
  {
    return raw_keyboard_getscancode() != -1;
  }

  int write(char const *s, unsigned long count) const
  {
    unsigned long c = count;
    while (c)
      {
        if (*s == 10)
          vga_putchar(13);
        vga_putchar(*s++);
        --c;
      }
    return count;
  }
};

class Dual_uart : public L4::Uart
{
private:
  L4::Uart *_u1, *_u2;

public:
  Dual_uart(L4::Uart *u1)
  : _u1(u1), _u2(0)
  {}

  void set_uart2(L4::Uart *u2)
  {
    _u2 = u2;
  }

  bool startup(L4::Io_register_block const *)
  {
    return true;
  }

  void shutdown()
  {
    _u1->shutdown();
    if (_u2)
      _u2->shutdown();
  }

  ~Dual_uart() {}
#if 0
  bool enable_rx_irq(bool e)
  {
    bool r1 = _u1->enable_rx_irq(e);
    bool r2 = _u2 ? _u2->enable_rx_irq(e) : false;
    return r1 && r2;
  }

  bool enable_tx_irq(bool e)
  {
    bool r1 = _u1->enable_tx_irq(e);
    bool r2 = _u2 ? _u2->enable_tx_irq(e) : false;
    return r1 && r2;
  }
#endif

  bool change_mode(Transfer_mode m, Baud_rate r)
  {
    bool r1 = _u1->change_mode(m, r);
    bool r2 = _u2 ? _u2->change_mode(m, r) : false;
    return r1 && r2;
  }

  int char_avail() const
  {
    return _u1->char_avail() || (_u2 && _u2->char_avail());
  }

  int get_char(bool blocking) const
  {
    int c;
    do
      {
        c = _u1->get_char(false);
        if (c == -1 && _u2)
          c = _u2->get_char(false);
      }
    while (blocking && c == -1);
    return c;
  }

  int write(char const *s, unsigned long count) const
  {
    int r = _u1->write(s, count);
    if (_u2)
      _u2->write(s, count);
    return r;
  }

};

l4util_mb_info_t *x86_bootloader_mbi;

namespace {

class Platform_x86 : public Platform_base
{
public:
  bool probe() { return true; }

  int init_uart(int com_port_or_base, int com_irq, Dual_uart *du)
  {
    base_critical_enter();

    switch (com_port_or_base)
      {
      case 1: com_port_or_base = 0x3f8;
              if (com_irq == -1)
                com_irq = 4;
              break;
      case 2: com_port_or_base = 0x2f8;
              if (com_irq == -1)
                com_irq = 3;
              break;
      case 3: com_port_or_base = 0x3e8; break;
      case 4: com_port_or_base = 0x2e8; break;
      }

    unsigned baudrate = 115200;
    static L4::Io_register_block_port uart_regs(com_port_or_base);
    static L4::Uart_16550 _uart(L4::Uart_16550::Base_rate_x86);
    if (   !_uart.startup(&uart_regs)
        || !_uart.change_mode(L4::Uart_16550::MODE_8N1, baudrate))
      {
        printf("Could not find or enable UART\n");
        base_critical_leave();
        return 1;
      }

    du->set_uart2(&_uart);

    kuart.access_type  = L4_kernel_options::Uart_type_ioport;
    kuart.irqno        = com_irq;
    kuart.base_address = com_port_or_base;
    kuart.baud         = baudrate;
    kuart_flags       |=   L4_kernel_options::F_uart_base
                         | L4_kernel_options::F_uart_baud;
    if (com_irq != -1)
      kuart_flags |= L4_kernel_options::F_uart_irq;

    base_critical_leave();
    return 0;
  }

  void init()
  {
    const char *s;
    int comport = -1;
    int comirq = -1;
    int pci = 0;

    static Uart_vga _vga;
    static Dual_uart du(&_vga);
    set_stdio_uart(&du);

    if ((s = check_arg(x86_bootloader_mbi, "-comirq")))
      {
        s += 8;
        comirq = strtoul(s, 0, 0);
      }

    if ((s = check_arg(x86_bootloader_mbi, "-comport")))
      {
        s += 9;
        if ((pci = !strncmp(s, "pci:", 4)))
          s += 4;

        comport = strtoul(s, 0, 0);
      }

    if (!check_arg(x86_bootloader_mbi, "-noserial"))
      {
        if (pci)
          {
            if (unsigned long port = search_pci_serial_devs(comport))
              comport = port;
            else
              comport = -1;

            printf("PCI IO port = %x\n", comport);
          }

        if (comport == -1)
          comport = 1;

        if (init_uart(comport, comirq, &du))
          printf("UART init failed\n");
      }
  }

  void setup_memory_map(l4util_mb_info_t *mbi,
                        Region_list *ram, Region_list *regions)
  {
    if (!(mbi->flags & L4UTIL_MB_MEM_MAP))
      {
        assert(mbi->flags & L4UTIL_MB_MEMORY);
        ram->add(Region::n(0, (mbi->mem_upper + 1024) << 10, ".ram",
                 Region::Ram));
      }
    else
      {
        l4util_mb_addr_range_t *mmap;
        l4util_mb_for_each_mmap_entry(mmap, mbi)
          {
            unsigned long long start = (unsigned long long)mmap->addr;
            unsigned long long end = (unsigned long long)mmap->addr + mmap->size;

            switch (mmap->type)
              {
              case 1:
                ram->add(Region::n(start, end, ".ram", Region::Ram));
                break;
              case 2:
              case 3:
              case 4:
                regions->add(Region::n(start, end, ".BIOS", Region::Arch, mmap->type));
                break;
              case 5:
                regions->add(Region::n(start, end, ".BIOS", Region::No_mem));
                break;
              default:
                break;
              }
          }
      }

    regions->add(Region::n(0, 0x1000, ".BIOS", Region::Arch, 0));


    // Quirks

    // Fix EBDA in conventional memory
    unsigned long p = *(l4_uint16_t *)0x40e << 4;

    if (p > 0x400)
      {
        unsigned long e = p + 1024;
        Region *r = ram->find(Region(p, e - 1));
        if (r)
          {
            if (e - 1 < r->end())
              ram->add(Region::n(e, r->end(), ".ram", Region::Ram));
            r->end(p);
          }
      }
  }
};
}

REGISTER_PLATFORM(Platform_x86);

