INTERFACE:

class Irq_base;

class Vkey
{
public:
  enum Echo_type { Echo_off = 0, Echo_on = 1, Echo_crnl = 2 };
};


// ---------------------------------------------------------------------------
IMPLEMENTATION:

#include "irq_chip.h"

static Irq_base *vkey_irq;

PUBLIC static
void
Vkey::irq(Irq_base *i)
{ vkey_irq = i; }

// ---------------------------------------------------------------------------
IMPLEMENTATION [debug && serial && !ux]:

#include <cstdio>

#include "config.h"
#include "cpu.h"
#include "globals.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "koptions.h"
#include "uart.h"

static Vkey::Echo_type vkey_echo;
static char     vkey_buffer[256];
static unsigned vkey_tail, vkey_head;
static Console *uart = Kconsole::console()->find_console(Console::UART);

PUBLIC static
void
Vkey::set_echo(Echo_type echo)
{
  vkey_echo = echo;
}

PRIVATE static
bool
Vkey::add(int c)
{
  bool hit = false;
  unsigned nh = (vkey_head + 1) % sizeof(vkey_buffer);
  unsigned oh = vkey_head;
  if (nh != vkey_tail)
    {
      vkey_buffer[vkey_head] = c;
      vkey_head = nh;
    }

  if (oh == vkey_tail)
    hit = true;

  if (vkey_echo == Vkey::Echo_crnl && c == '\r')
    c = '\n';

  if (vkey_echo)
    putchar(c);

  return hit;
}

PRIVATE static
bool
Vkey::add(const char *seq)
{
  bool hit = false;
  for (; *seq; ++seq)
    hit |= add(*seq);
  return hit;
}

PRIVATE static
void
Vkey::trigger()
{
  if (vkey_irq)
    vkey_irq->hit(0);
}

PUBLIC static
void
Vkey::add_char(int v)
{
  if (add(v))
    trigger();
}

PUBLIC static
int
Vkey::check_()
{
  if (!uart)
    return 1;

  int  ret = 1;
  bool hit = false;

  // disable last branch recording, branch trace recording ...
  Cpu::cpus.cpu(current_cpu()).debugctl_disable();

  while (1)
    {
      int c = uart->getchar(false);

      if (c == -1)
	break;

      if ((c == KEY_ESC) && (Koptions::o()->opt(Koptions::F_serial_esc)))
	{
	  ret = 0;  // break into kernel debugger
	  break;
	}

      switch (c)
	{
	case KEY_CURSOR_UP:    hit |= add("\033[A"); break;
	case KEY_CURSOR_DOWN:  hit |= add("\033[B"); break;
	case KEY_CURSOR_LEFT:  hit |= add("\033[D"); break;
	case KEY_CURSOR_RIGHT: hit |= add("\033[C"); break;
	case KEY_CURSOR_HOME:  hit |= add("\033[1~"); break;
	case KEY_CURSOR_END:   hit |= add("\033[4~"); break;
	case KEY_PAGE_UP:      hit |= add("\033[5~"); break;
	case KEY_PAGE_DOWN:    hit |= add("\033[6~"); break;
	case KEY_INSERT:       hit |= add("\033[2~"); break;
	case KEY_DELETE:       hit |= add("\033[3~"); break;
	case KEY_F1:           hit |= add("\033OP"); break;
	case KEY_BACKSPACE:    hit |= add(127); break;
	case KEY_TAB:          hit |= add(9); break;
	case KEY_ESC:          hit |= add(27); break;
	case KEY_RETURN:       hit |= add(13); break;
	default:               hit |= add(c); break;
	}
    }

  if (hit)
    trigger();

  // Hmmm, we assume that a console with the UART flag set is of type Uart
  if (Config::serial_esc == Config::SERIAL_ESC_IRQ)
    static_cast<Uart*>(uart)->enable_rcv_irq();

  // reenable debug stuff (undo debugctl_disable)
  Cpu::cpus.cpu(current_cpu()).debugctl_enable();

  return ret;
}

PUBLIC static
int
Vkey::get()
{
  if (vkey_tail != vkey_head)
    return vkey_buffer[vkey_tail];

  return -1;
}

PUBLIC static
void
Vkey::clear()
{
  if (vkey_tail != vkey_head)
    vkey_tail = (vkey_tail + 1) % sizeof(vkey_buffer);
}

//----------------------------------------------------------------------------
IMPLEMENTATION [!debug || !serial || ux]:

PUBLIC static inline
void
Vkey::set_echo(Echo_type)
{}

PUBLIC static inline
void
Vkey::clear()
{}

PUBLIC static inline
void
Vkey::add_char(int)
{}

//----------------------------------------------------------------------------
IMPLEMENTATION [debug && (!serial || ux)]:

#include "kernel_console.h"

PUBLIC static
int
Vkey::get()
{
  return Kconsole::console()->getchar(0);
}

//----------------------------------------------------------------------------
IMPLEMENTATION [!debug && serial]:

#include "kernel_console.h"

static Console *uart = Kconsole::console()->find_console(Console::UART);

PUBLIC static
int
Vkey::get()
{
  return uart->getchar(false);
}

//----------------------------------------------------------------------------
IMPLEMENTATION[!debug && !serial]:

PUBLIC static
int
Vkey::get()
{ return -1; }

//----------------------------------------------------------------------------
IMPLEMENTATION[!debug || !serial]:

PUBLIC static inline
int
Vkey::check_(int = -1)
{ return 0; }
