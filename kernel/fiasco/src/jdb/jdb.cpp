INTERFACE:

#include "l4_types.h"
#include "jdb_core.h"
#include "jdb_handler_queue.h"
#include "per_cpu_data.h"

class Context;
class Thread;
class Push_console;

class Jdb_entry_frame;

class Jdb : public Jdb_core
{
public:
  static Per_cpu<Jdb_entry_frame*> entry_frame;
  static unsigned current_cpu;
  static Per_cpu<void (*)(unsigned, void *)> remote_func;
  static Per_cpu<void *> remote_func_data;
  static Per_cpu<bool> remote_func_running;

  static int FIASCO_FASTCALL enter_jdb(Jdb_entry_frame *e, unsigned cpu);
  static void cursor_end_of_screen();
  static void cursor_home();
  static void printf_statline(const char *prompt, const char *help,
                              const char *format, ...)
  __attribute__((format(printf, 3, 4)));
  static void save_disable_irqs(unsigned cpu);
  static void restore_irqs(unsigned cpu);

private:
  Jdb();			// default constructors are undefined
  Jdb(const Jdb&);

  static char hide_statline;
  static char last_cmd;
  static char next_cmd;
  static Per_cpu<char[81]> error_buffer;
  static bool was_input_error;

  static Thread  *current_active;

  static const char *toplevel_cmds;
  static const char *non_interactive_cmds;

  // state for traps in JDB itself
  static Per_cpu<bool> running;
  static bool in_service;
  static bool leave_barrier;
  static unsigned long cpus_in_debugger;
  static bool never_break;
  static bool jdb_active;

  static void enter_trap_handler(unsigned cpu);
  static void leave_trap_handler(unsigned cpu);
  static bool handle_conditional_breakpoint(unsigned cpu);
  static void handle_nested_trap(Jdb_entry_frame *e);
  static bool handle_user_request(unsigned cpu);
  static bool handle_debug_traps(unsigned cpu);
  static bool test_checksums();

public:
  static Jdb_handler_queue jdb_enter;
  static Jdb_handler_queue jdb_leave;

  // esc sequences for highligthing
  static char  esc_iret[];
  static char  esc_bt[];
  static char  esc_emph[];
  static char  esc_emph2[];
  static char  esc_mark[];
  static char  esc_line[];
  static char  esc_symbol[];

};

//---------------------------------------------------------------------------
IMPLEMENTATION:

#include <cstdio>
#include <cstring>
#include <simpleio.h>

#include "config.h"
#include "delayloop.h"
#include "feature.h"
#include "jdb_core.h"
#include "jdb_entry_frame.h"
#include "jdb_screen.h"
#include "kernel_console.h"
#include "processor.h"
#include "push_console.h"
#include "static_init.h"
#include "keycodes.h"

KIP_KERNEL_FEATURE("jdb");

Jdb_handler_queue Jdb::jdb_enter;
Jdb_handler_queue Jdb::jdb_leave;

DEFINE_PER_CPU Per_cpu<char[81]> Jdb::error_buffer;
char Jdb::next_cmd;			// next global command to execute
char Jdb::last_cmd;

char Jdb::hide_statline;		// show status line on enter_kdebugger
DEFINE_PER_CPU Per_cpu<Jdb_entry_frame*> Jdb::entry_frame;
unsigned Jdb::current_cpu;              // current CPU JDB is running on
Thread *Jdb::current_active;		// current running thread
bool Jdb::was_input_error;		// error in command sequence

DEFINE_PER_CPU Per_cpu<void (*)(unsigned, void *)> Jdb::remote_func;
DEFINE_PER_CPU Per_cpu<void *> Jdb::remote_func_data;
DEFINE_PER_CPU Per_cpu<bool> Jdb::remote_func_running;

// holds all commands executable in top level (regardless of current mode)
const char *Jdb::toplevel_cmds = "j_";

// a short command must be included in this list to be enabled for non-
// interactive execution
const char *Jdb::non_interactive_cmds = "bEIJLMNOPSU^Z";

DEFINE_PER_CPU Per_cpu<bool> Jdb::running;	// JDB is already running
bool Jdb::never_break;		// never enter JDB
bool Jdb::jdb_active;
bool Jdb::in_service;
bool Jdb::leave_barrier;
unsigned long Jdb::cpus_in_debugger;


PUBLIC static
bool
Jdb::cpu_in_jdb(unsigned cpu)
{ return Cpu::online(cpu) && running.cpu(cpu); }


PUBLIC static
template< typename Func >
void
Jdb::foreach_cpu(Func const &f)
{
  for (unsigned i = 0; i < Config::Max_num_cpus; ++i)
    {
      if (!Cpu::online(i) || !running.cpu(i))
	continue;
      f(i);
    }
}

PUBLIC static
template< typename Func >
bool
Jdb::foreach_cpu(Func const &f, bool positive)
{
  bool r = positive;
  for (unsigned i = 0; i < Config::Max_num_cpus; ++i)
    {
      if (!Cpu::online(i) || !running.cpu(i))
	continue;

      bool res = f(i);

      if (positive)
	r = r && res;
      else
	r = r || res;
    }

  return r;
}

PUBLIC static inline
void
Jdb::set_next_cmd(char cmd)
{ next_cmd = cmd; }

PUBLIC static inline
int
Jdb::was_last_cmd()
{ return last_cmd; }

PUBLIC static inline
int
Jdb::get_next_cmd()
{ return next_cmd; }

/** Command aborted. If we are interpreting a debug command like
 *  enter_kdebugger("*#...") this is an error
 */
PUBLIC
static void
Jdb::abort_command()
{
  cursor(Jdb_screen::height(), 6);
  clear_to_eol();

  was_input_error = true;
}


// go to bottom of screen and print some text in the form "jdb: ..."
// if no text follows after the prompt, prefix the current thread number
IMPLEMENT
void
Jdb::printf_statline(const char *prompt, const char *help,
                     const char *format, ...)
{
  cursor(Jdb_screen::height(), 1);
  unsigned w = Jdb_screen::width();
  prompt_start();
  if (prompt)
    {
      putstr(prompt);
      putstr(": ");
      w -= strlen(prompt) + 2;
    }
  else
    {
      Jdb::prompt();
      w -= Jdb::prompt_len();
    }
  prompt_end();
  // work around for ealier gccs complaining about "empty format strings"
  if (format && (format[0] != '_' || format[1] != '\0'))
    {
      char s[w];
      va_list list;
      va_start(list, format);
      vsnprintf(s, sizeof(s), format, list);
      va_end(list);
      s[sizeof(s) - 1] = 0;
      putstr(s);
      w -= print_len(s);
    }
  if (help && print_len(help) < w)
    printf("%*.*s", w, w, help);
  else
    clear_to_eol();
}

PUBLIC static
bool Jdb::is_toplevel_cmd(char c)
{
  char cm[] = {c, 0};
  Jdb_core::Cmd cmd = Jdb_core::has_cmd(cm);

  if (cmd.cmd || (0 != strchr(toplevel_cmds, c)))
    {
      set_next_cmd(c);
      return true;
    }

  return false;
}


PUBLIC static 
int
Jdb::execute_command(const char *s, int first_char = -1)
{
  Jdb_core::Cmd cmd = Jdb_core::has_cmd(s);

  if (cmd.cmd)
    return Jdb_core::exec_cmd(cmd, 0, first_char) == 2 ? 1 : 0;

  return 0;
}

PUBLIC static
Push_console *
Jdb::push_cons()
{
  static Push_console c;
  return &c;
}

// Interprete str as non interactive commands for Jdb. We allow mostly 
// non-interactive commands here (e.g. we don't allow d, t, l, u commands)
PRIVATE static
int
Jdb::execute_command_ni(Space *task, char const *str, int len = 1000)
{
  char tmp = 0;
  for (; len && peek(str, task, tmp) && tmp; ++str, --len)
    if ((unsigned char)tmp != 0xff)
      push_cons()->push(tmp);

  if ((unsigned char)tmp != 0xff)
    push_cons()->push('_'); // terminating return


  // prevent output of sequences
  Kconsole::console()->change_state(0, 0, ~Console::OUTENABLED, 0);

  for (;;)
    {
      int c = getchar();

      was_input_error = true;
      if (0 != strchr(non_interactive_cmds, c))
	{
	  char _cmd[] = {(char)c, 0};
	  Jdb_core::Cmd cmd = Jdb_core::has_cmd(_cmd);

	  if (cmd.cmd)
	    {
	      if (Jdb_core::exec_cmd (cmd, 0) != 3)
                was_input_error = false;
	    }
	}

      if (c == KEY_RETURN || c == ' ' || was_input_error)
	{
	  push_cons()->flush();
	  // re-enable all consoles but GZIP
	  Kconsole::console()->change_state(0, Console::GZIP,
					    ~0U, Console::OUTENABLED);
	  return c == KEY_RETURN || c == ' ';
	}
    }
}

PRIVATE static
bool
Jdb::input_short_mode(Jdb::Cmd *cmd, char const **args, int &cmd_key)
{
  *args = 0;
  for (;;)
    {
      int c;
      do
	{
	  if ((c = get_next_cmd()))
	    set_next_cmd(0);
	  else
	    c = getchar();
	}
      while (c < ' ' && c != KEY_RETURN);

      if (c == KEY_F1)
	c = 'h';

      printf("\033[K%c", c); // clreol + print key

      char cmd_buffer[2] = { (char)c, 0 };

      *cmd = Jdb_core::has_cmd(cmd_buffer);
      if (cmd->cmd)
	{
	  cmd_key = c;
	  return false; // do not leave the debugger
	}
      else if (!handle_special_cmds(c))
	return true; // special command triggered a JDB leave
      else if (c == KEY_RETURN)
	{
	  hide_statline = false;
	  cmd_key = c;
	  return false;
	}
    }
}


class Cmd_buffer
{
private:
  unsigned  _l;
  char _b[256];

public:
  Cmd_buffer() {}
  char *buffer() { return _b; }
  int len() const { return _l; }
  void flush() { _l = 0; _b[0] = 0; }
  void cut(int l) 
  {
    if (l < 0)
      l = _l + l;

    if (l >= 0 && (unsigned)l < _l)
      {
	_l = l;
	_b[l] = 0;
      }
  }

  void append(int c) { if (_l + 1 < sizeof(_b)) { _b[_l++] = c; _b[_l] = 0; } }
  void append(char const *str, int len)
  {
    if (_l + len >= sizeof(_b))
      len = sizeof(_b) - _l - 1;

    memcpy(_b + _l, str, len);
    _l += len;
    _b[_l] = 0;
  }

  void overlay(char const *str, unsigned len)
  {
    if (len + 1 > sizeof(_b))
      len = sizeof(_b) - 1;

    if (len < _l)
      return;

    str += _l;
    len -= _l;

    memcpy(_b + _l, str, len);
    _l = len + _l;
  }

};


PRIVATE static
bool
Jdb::input_long_mode(Jdb::Cmd *cmd, char const **args)
{
  static Cmd_buffer buf;
  buf.flush();
  for (;;)
    {
      int c = getchar();

      switch (c)
	{
	case KEY_BACKSPACE:
	  if (buf.len() > 0)
	    {
	      cursor(Cursor_left);
	      clear_to_eol();
	      buf.cut(-1);
	    }
	  continue;

	case ' ':
	  if (buf.len() == 0)
	    continue;
	  break;

	case KEY_TAB:
	    {
	      bool multi_match = false;
	      *cmd = Jdb_core::complete_cmd(buf.buffer(), multi_match);
	      if (cmd->cmd && multi_match)
		{
		  printf("\n");
		  unsigned prefix_len = Jdb_core::print_alternatives(buf.buffer());
		  print_prompt();
		  buf.overlay(cmd->cmd->cmd, prefix_len);
		  putnstr(buf.buffer(), buf.len());
		}
	      else if (cmd->cmd)
		{
		  putstr(cmd->cmd->cmd + buf.len());
		  putchar(' ');
		  buf.overlay(cmd->cmd->cmd, strlen(cmd->cmd->cmd));
		  buf.append(' ');
		}
	      continue;
	    }
	  break;

	case KEY_RETURN:
	  puts("");
	  if (!buf.len())
	    {
	      hide_statline = false;
	      cmd->cmd = 0;
	      return false;
	    }
	  break;

	default:
	  buf.append(c);
	  printf("\033[K%c", c);
	  continue;
	}

      *cmd = Jdb_core::has_cmd(buf.buffer());
      if (cmd->cmd)
	{
	  unsigned cmd_len = strlen(cmd->cmd->cmd);
	  *args = buf.buffer() + cmd_len;
	  while (**args == ' ')
	    ++(*args);
	  return false; // do not leave the debugger
	}
      else
	{
	  printf("unknown command: '%s'\n", buf.buffer());
	  print_prompt();
	  buf.flush();
	}
    }
}

PRIVATE static
int
Jdb::execute_command()
{
  char const *args;
  Jdb_core::Cmd cmd(0,0);
  bool leave;
  int cmd_key;

  if (short_mode)
    leave = input_short_mode(&cmd, &args, cmd_key);
  else
    leave = input_long_mode(&cmd, &args);

  if (leave)
    return 0;

  if (cmd.cmd)
    {
      int ret = Jdb_core::exec_cmd( cmd, args );

      if (!ret)
	hide_statline = false;

      last_cmd = cmd_key;
      return ret;
    }

  last_cmd = 0;
  return 1;
}

PRIVATE static
bool
Jdb::open_debug_console(unsigned cpu)
{
  in_service = 1;
  save_disable_irqs(cpu);
  if (cpu == 0)
    jdb_enter.execute();

  if (!stop_all_cpus(cpu))
    return false; // CPUs other than 0 never become interacitve

  if (!Jdb_screen::direct_enabled())
    Kconsole::console()->
      change_state(Console::DIRECT, 0, ~Console::OUTENABLED, 0);

  return true;
}


PRIVATE static
void
Jdb::close_debug_console(unsigned cpu)
{
  Proc::cli();
  Mem::barrier();
  if (cpu == 0)
    {
      running.cpu(cpu) = 0;
      // eat up input from console
      while (Kconsole::console()->getchar(false)!=-1)
	;

      Kconsole::console()->
	change_state(Console::DIRECT, 0, ~0UL, Console::OUTENABLED);

      in_service = 0;
      leave_wait_for_others();
      jdb_leave.execute();
    }

  Mem::barrier();
  restore_irqs(cpu);
}

PUBLIC static
void
Jdb::remote_work(unsigned cpu, void (*func)(unsigned, void *), void *data,
                 bool sync = true)
{
  if (cpu == 0)
    func(cpu, data);
  else
    {
      while (1)
	{
	  Mem::barrier();
	  if (!Jdb::remote_func_running.cpu(cpu))
	    break;
	  Proc::pause();
	}

      Jdb::remote_func_running.cpu(cpu) = 1;
      Jdb::remote_func_data.cpu(cpu) = data;
      Mem::barrier();
      set_monitored_address(&Jdb::remote_func.cpu(cpu), func);
      Mem::barrier();

      while (sync)
	{
	  Mem::barrier();
	  if (!Jdb::remote_func_running.cpu(cpu))
	    break;
	  Proc::pause();
	}
    }
}

PUBLIC
static int
Jdb::getchar(void)
{
  int res = Kconsole::console()->getchar();
  check_for_cpus(false);
  return res;
}

IMPLEMENT
void Jdb::cursor_home()
{
  putstr("\033[H");
}

IMPLEMENT
void Jdb::cursor_end_of_screen()
{
  putstr("\033[127;1H");
}

//-------- pretty print functions ------------------------------
PUBLIC static
void
Jdb::write_ll_ns(Signed64 ns, char *buf, int maxlen, bool sign)
{
  Unsigned64 uns = (ns < 0) ? -ns : ns;

  if (uns >= 3600000000000000ULL)
    {
      snprintf(buf, maxlen, ">999 h ");
      return;
    }

  if (maxlen && sign)
    {
      *buf++ = (ns < 0) ? '-' : (ns == 0) ? ' ' : '+';
      maxlen--;
    }

  if (uns >= 60000000000000ULL)
    {
      // 1000min...999h
      Mword _h  = uns / 3600000000000ULL;
      Mword _m  = (uns % 3600000000000ULL) / 60000000000ULL;
      snprintf(buf, maxlen, "%3lu:%02lu h  ", _h, _m);
      return;
    }

  if (uns >= 1000000000000ULL)
    {
      // 1000s...999min
      Mword _m  = uns / 60000000000ULL;
      Mword _s  = (uns % 60000000000ULL) / 1000ULL;
      snprintf(buf, maxlen, "%3lu:%02lu M  ", _m, _s);
      return;
    }

  if (uns >= 1000000000ULL)
    {
      // 1...1000s
      Mword _s  = uns / 1000000000ULL;
      Mword _ms = (uns % 1000000000ULL) / 1000000ULL;
      snprintf(buf, maxlen, "%3lu.%03lu s ", _s, _ms);
      return;
    }

  if (uns >= 1000000)
    {
      // 1...1000ms
      Mword _ms = uns / 1000000UL;
      Mword _us = (uns % 1000000UL) / 1000UL;
      snprintf(buf, maxlen, "%3lu.%03lu ms", _ms, _us);
      return;
    }

  if (uns == 0)
    {
      snprintf(buf, maxlen, "  0       ");
      return;
    }

  Console* gzip = Kconsole::console()->find_console(Console::GZIP);
  Mword _us = uns / 1000UL;
  Mword _ns = uns % 1000UL;
  snprintf(buf, maxlen, "%3lu.%03lu %c ", _us, _ns,
           gzip && gzip->state() & Console::OUTENABLED
             ? '\265' 
             : Config::char_micro);
}

PUBLIC static
void
Jdb::write_ll_hex(Signed64 x, char *buf, int maxlen, bool sign)
{
  // display 40 bits
  Unsigned64 xu = (x < 0) ? -x : x;

  if (sign)
    snprintf(buf, maxlen, "%s%03lx" L4_PTR_FMT,
			  (x < 0) ? "-" : (x == 0) ? " " : "+",
			  (Mword)((xu >> 32) & 0xfff), (Mword)xu);
  else
    snprintf(buf, maxlen, "%04lx" L4_PTR_FMT,
			  (Mword)((xu >> 32) & 0xffff), (Mword)xu);
}

PUBLIC static
void
Jdb::write_ll_dec(Signed64 x, char *buf, int maxlen, bool sign)
{
  Unsigned64 xu = (x < 0) ? -x : x;

  // display no more than 11 digits
  if (xu >= 100000000000ULL)
    {
      snprintf(buf, maxlen, "%12s", ">= 10^11");
      return;
    }

  if (sign && x != 0)
    snprintf(buf, maxlen, "%+12lld", x);
  else
    snprintf(buf, maxlen, "%12llu", xu);
}

PUBLIC static inline
Thread*
Jdb::get_current_active()
{
  return current_active;
}

PUBLIC static inline
Jdb_entry_frame*
Jdb::get_entry_frame(unsigned cpu)
{
  return entry_frame.cpu(cpu);
}

/// handling of standard cursor keys (Up/Down/PgUp/PgDn)
PUBLIC static
int
Jdb::std_cursor_key(int c, Mword cols, Mword lines, Mword max_absy, Mword *absy,
                    Mword *addy, Mword *addx, bool *redraw)
{
  switch (c)
    {
    case KEY_CURSOR_LEFT:
    case 'h':
      if (addx)
	{
	  if (*addx > 0)
	    (*addx)--;
	  else if (*addy > 0)
	    {
	      (*addy)--;
	      *addx = cols - 1;
	    }
	  else if (*absy > 0)
	    {
	      (*absy)--;
	      *addx = cols - 1;
	      *redraw = true;
	    }
	}
      else
	return 0;
      break;
    case KEY_CURSOR_RIGHT:
    case 'l':
      if (addx)
	{   
	  if (*addx < cols - 1)
	    (*addx)++;
	  else if (*addy < lines - 1)
	    {
	      (*addy)++; 
	      *addx = 0;
	    }
	  else if (*absy < max_absy)
	    {
	      (*absy)++;
	      *addx = 0;
	      *redraw = true;
	    }
	}
      else
	return 0;
      break;
    case KEY_CURSOR_UP:
    case 'k':
      if (*addy > 0)
	(*addy)--;
      else if (*absy > 0)
	{
	  (*absy)--;
	  *redraw = true;
	}
      break;
    case KEY_CURSOR_DOWN:
    case 'j':
      if (*addy < lines-1)
	(*addy)++;
      else if (*absy < max_absy)
	{
	  (*absy)++;
	  *redraw = true;
	}
      break;
    case KEY_CURSOR_HOME:
    case 'H':
      *addy = 0;
      if (addx)
	*addx = 0;
      if (*absy > 0)
	{
	  *absy = 0;
	  *redraw = true;
	}
      break;
    case KEY_CURSOR_END:
    case 'L':
      *addy = lines-1;
      if (addx)
	*addx = cols - 1;
      if (*absy < max_absy)
	{
	  *absy = max_absy;
	  *redraw = true;
	}
      break;
    case KEY_PAGE_UP:
    case 'K':
      if (*absy >= lines)
	{
	  *absy -= lines;
	  *redraw = true;
	}
      else
	{
	  if (*absy > 0)
	    {
	      *absy = 0;
	      *redraw = true;
	    }
	  else if (*addy > 0)
	    *addy = 0;
	  else if (addx)
	    *addx = 0;
	}
      break;
    case KEY_PAGE_DOWN:
    case 'J':
      if (*absy+lines-1 < max_absy)
	{
	  *absy += lines;
	  *redraw = true;
	}
      else
	{
	  if (*absy < max_absy)
	    {
	      *absy = max_absy;
	      *redraw = true;
	    }
	  else if (*addy < lines-1)
      	    *addy = lines-1;
	  else if (addx)
	    *addx = cols - 1;
	}
      break;
    default:
      return 0;
    }

  return 1;
}

PUBLIC static inline
Space *
Jdb::get_task(unsigned cpu)
{
  if (!get_thread(cpu))
    return 0;
  else
    return get_thread(cpu)->space();
}


//
// memory access wrappers
//

PUBLIC static
template< typename T >
bool
Jdb::peek(T const *addr, Space *task, T &value)
{
  // use an Mword here instead of T as some implementations of peek_task use
  // an Mword in their operation which is potentially bigger than T
  // XXX: should be fixed
  Mword tmp;
  bool ret = peek_task((Address)addr, task, &tmp, sizeof(T)) == 0;
  value = tmp;
  return ret;
}

PUBLIC static
template< typename T >
bool
Jdb::poke(T *addr, Space *task, T const &value)
{ return poke_task((Address)addr, task, &value, sizeof(T)) == 0; }


class Jdb_base_cmds : public Jdb_module
{
public:
  Jdb_base_cmds() FIASCO_INIT;
};

static Jdb_base_cmds jdb_base_cmds INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

PUBLIC
Jdb_module::Action_code
Jdb_base_cmds::action (int cmd, void *&, char const *&, int &)
{
  if (cmd!=0)
    return NOTHING;

  Jdb_core::short_mode = !Jdb_core::short_mode;
  printf("\ntoggle mode: now in %s command mode (use %s) to switch back\n",
         Jdb_core::short_mode ? "short" : "long",
         Jdb_core::short_mode ? "*" : "mode");
  return NOTHING;
}

PUBLIC
int
Jdb_base_cmds::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd const *
Jdb_base_cmds::cmds() const
{
  static Cmd cs[] =
    { { 0, "*", "mode", "", "*|mode\tswitch long and short command mode",
	(void*)0 } };

  return cs;
}

IMPLEMENT
Jdb_base_cmds::Jdb_base_cmds()
  : Jdb_module("GENERAL")
{}


//---------------------------------------------------------------------------
IMPLEMENTATION [ux]:

PRIVATE inline static void Jdb::rcv_uart_enable() {}

//---------------------------------------------------------------------------
IMPLEMENTATION [!ux]:

#include "kernel_uart.h"

PRIVATE inline static
void
Jdb::rcv_uart_enable()
{
  if (Config::serial_esc == Config::SERIAL_ESC_IRQ)
    Kernel_uart::enable_rcv_irq();
}

//---------------------------------------------------------------------------
IMPLEMENTATION:

#include "ipi.h"
#include "logdefs.h"

char Jdb::esc_iret[]     = "\033[36;1m";
char Jdb::esc_bt[]       = "\033[31m";
char Jdb::esc_emph[]     = "\033[33;1m";
char Jdb::esc_emph2[]    = "\033[32;1m";
char Jdb::esc_mark[]     = "\033[35;1m";
char Jdb::esc_line[]     = "\033[37m";
char Jdb::esc_symbol[]   = "\033[33;1m";





IMPLEMENT int
Jdb::enter_jdb(Jdb_entry_frame *e, unsigned cpu)
{
  if (e->debug_ipi())
    {
      if (!remote_work_ipi_process(cpu))
        return 0;
      if (!in_service)
	return 0;
    }

  enter_trap_handler(cpu);

  if (handle_conditional_breakpoint(cpu))
    {
      // don't enter debugger, only logged breakpoint
      leave_trap_handler(cpu);
      return 0;
    }

  if (!running.cpu(cpu))
    entry_frame.cpu(cpu) = e;

  volatile bool really_break = true;

  static jmp_buf recover_buf;
  static Jdb_entry_frame nested_trap_frame;

  if (running.cpu(cpu))
    {
      nested_trap_frame = *e;

      // Since we entered the kernel debugger a second time,
      // Thread::nested_trap_recover
      // has a value of 2 now. We don't leave this function so correct the
      // entry counter
      Thread::nested_trap_recover.cpu(cpu)--;

      longjmp(recover_buf, 1);
    }

  // all following exceptions are handled by jdb itself
  running.cpu(cpu) = true;

  if (!open_debug_console(cpu))
    { // not on the master CPU just wait
      close_debug_console(cpu);
      leave_trap_handler(cpu);
      return 0;
    }

  Jdb::current_cpu = cpu;
  // check for int $3 user debugging interface
  if (foreach_cpu(&handle_user_request, true))
    {
      close_debug_console(cpu);
      leave_trap_handler(cpu);
      return 0;
    }

  hide_statline = false;

  // clear error message
  *error_buffer.cpu(cpu) = '\0';

  really_break = foreach_cpu(&handle_debug_traps, false);

  while (setjmp(recover_buf))
    {
      // handle traps which occured while we are in Jdb
      Kconsole::console()->end_exclusive(Console::GZIP);
      handle_nested_trap(&nested_trap_frame);
    }

  if (!never_break && really_break) 
    {
      // determine current task/thread from stack pointer
      update_prompt();

      LOG_MSG(current_active, "=== enter jdb ===");

      do
	{
	  screen_scroll(1, Jdb_screen::height());
	  if (!hide_statline)
	    {
	      cursor(Jdb_screen::height(), 1);
	      printf("\n%s%s    %.*s\033[m      \n",
	             esc_prompt,
	             test_checksums()
	               ? ""
	               : "    WARNING: Fiasco kernel checksum differs -- "
	                 "read-only data has changed!\n",
	             Jdb_screen::width()-11,
	             Jdb_screen::Line);
	      for (unsigned i = 0; i < Config::Max_num_cpus; ++i)
		if (Cpu::online(i))
		  {
		    if (running.cpu(i))
		      printf("    CPU%2u [" L4_PTR_FMT "]: %s\n", i,
		             entry_frame.cpu(i)->ip(), error_buffer.cpu(i));
		    else
		      printf("    CPU%2u: is not in JDB (not responding)\n", i);
		  }
	      hide_statline = true;
	    }

	  printf_statline(0, 0, "_");

	} while (execute_command());

      // reset scrolling region of serial terminal
      screen_scroll(1,127);

      // reset cursor
      blink_cursor(Jdb_screen::height(), 1);

      // goto end of screen
      Jdb::cursor(127, 1);
    }

  // reenable interrupts
  close_debug_console(cpu);

  rcv_uart_enable();

  leave_trap_handler(cpu);
  return 0;
}


//--------------------------------------------------------------------------
IMPLEMENTATION [!mp]:

PRIVATE static
bool
Jdb::stop_all_cpus(unsigned /*current_cpu*/)
{ return true; }

PRIVATE
static
void
Jdb::leave_wait_for_others()
{}

PRIVATE static
bool
Jdb::check_for_cpus(bool)
{ return true; }

PRIVATE static inline
int
Jdb::remote_work_ipi_process(unsigned)
{ return 1; }


//---------------------------------------------------------------------------
INTERFACE [mp]:

#include "spin_lock.h"

EXTENSION class Jdb
{
  // remote call
  static Spin_lock<> _remote_call_lock;
  static void (*_remote_work_ipi_func)(unsigned, void *);
  static void *_remote_work_ipi_func_data;
  static unsigned long _remote_work_ipi_done;
};

//--------------------------------------------------------------------------
IMPLEMENTATION [mp]:

void (*Jdb::_remote_work_ipi_func)(unsigned, void *);
void *Jdb::_remote_work_ipi_func_data;
unsigned long Jdb::_remote_work_ipi_done;
Spin_lock<> Jdb::_remote_call_lock;

PRIVATE static
bool
Jdb::check_for_cpus(bool try_nmi)
{
  enum { Max_wait_cnt = 1000 };
  for (unsigned c = 1; c < Config::Max_num_cpus; ++c)
    {
      if (Cpu::online(c) && !running.cpu(c))
	Ipi::send(Ipi::Debug, 0, c);
    }
  Mem::barrier();
retry:
  unsigned long wait_cnt = 0;
  for (;;)
    {
      bool all_there = true;
      cpus_in_debugger = 0;
      // skip boot cpu 0
      for (unsigned c = 1; c < Config::Max_num_cpus; ++c)
	{
	  if (Cpu::online(c))
	    {
	      if (!running.cpu(c))
		all_there = false;
	      else
		++cpus_in_debugger;
	    }
	}

      if (!all_there)
	{
	  Proc::pause();
	  Mem::barrier();
	  if (++wait_cnt == Max_wait_cnt)
	    break;
	  Delay::delay(1);
	  continue;
	}

      break;
    }

  bool do_retry = false;
  for (unsigned c = 1; c < Config::Max_num_cpus; ++c)
    {
      if (Cpu::online(c))
	{
	  if (!running.cpu(c))
	    {
	      printf("JDB: CPU %d: is not responding ... %s\n",c,
		     try_nmi ? "trying NMI" : "");
	      if (try_nmi)
		{
		  do_retry = true;
		  send_nmi(c);
		}
	    }
	}
    }
  if (do_retry)
    {
      try_nmi = false;
      goto retry;
    }
  // All CPUs entered JDB, so go on and become interactive
  return true;
}

PRIVATE static
bool
Jdb::stop_all_cpus(unsigned current_cpu)
{
  enum { Max_wait_cnt = 1000 };
  // JDB allways runs on CPU 0, if any other CPU enters the debugger
  // CPU 0 is notified to do enter the debugger too
  if (current_cpu == 0)
    {
      // I'm CPU 0 stop all other CPUs and wait for them to enter the JDB
      jdb_active = 1;
      Mem::barrier();
      check_for_cpus(true);
      // All CPUs entered JDB, so go on and become interactive
      return true;
    }
  else
    {
      // Huh, not CPU 0, so notify CPU 0 to enter JDB too
      // The notification is ignored if CPU 0 is already within JDB
      jdb_active = true;
      Ipi::send(Ipi::Debug, current_cpu, 0);

      unsigned long wait_count = Max_wait_cnt;
      while (!running.cpu(0) && wait_count)
	{
	  Proc::pause();
          Delay::delay(1);
	  Mem::barrier();
	  --wait_count;
	}

      if (wait_count == 0)
	send_nmi(0);

      // Wait for messages from CPU 0
      while ((volatile bool)jdb_active)
	{
	  Mem::barrier();
	  void (**func)(unsigned, void *) = &remote_func.cpu(current_cpu);
	  void (*f)(unsigned, void *);

	  if ((f = monitor_address(current_cpu, func)))
	    {
	      // Execute functions from queued from another CPU
	      *func = 0;
	      f(current_cpu, remote_func_data.cpu(current_cpu));
	      Mem::barrier();
	      remote_func_running.cpu(current_cpu) = 0;
	      Mem::barrier();
	    }
	  Proc::pause();
	}

      // This CPU defacto left JDB
      running.cpu(current_cpu) = 0;

      // Signal CPU 0, that we are ready to leve the debugger
      // This is the second door of the airlock
      atomic_mp_add(&cpus_in_debugger, -1UL);

      // Wait for CPU 0 to leave us out
      while ((volatile bool)leave_barrier)
	{
	  Mem::barrier();
	  Proc::pause();
	}

      // CPU 0 signaled us to leave JDB
      return false;
    }
}

PRIVATE
static
void
Jdb::leave_wait_for_others()
{
  leave_barrier = 1;
  jdb_active = 0;
  Mem::barrier();
  for (;;)
    {
      bool all_there = true;
      for (unsigned c = 0; c < Config::Max_num_cpus; ++c)
	{
	  if (Cpu::online(c) && running.cpu(c))
	    {
	      // notify other CPU
              set_monitored_address(&Jdb::remote_func.cpu(c),
                                    (void (*)(unsigned, void *))0);
//	      printf("JDB: wait for CPU[%2u] to leave\n", c);
	      all_there = false;
	    }
	}

      if (!all_there)
	{
	  Proc::pause();
	  Mem::barrier();
	  continue;
	}

      break;
    }

  while ((volatile unsigned long)cpus_in_debugger)
    {
      Mem::barrier();
      Proc::pause();
    }

  Mem::barrier();
  leave_barrier = 0;
}

// The remote_work_ipi* functions are for the IPI round-trip benchmark (only)
PRIVATE static
int
Jdb::remote_work_ipi_process(unsigned cpu)
{
  if (_remote_work_ipi_func)
    {
      _remote_work_ipi_func(cpu, _remote_work_ipi_func_data);
      Mem::barrier();
      _remote_work_ipi_done = 1;
      return 0;
    }
  return 1;
}

PUBLIC static
bool
Jdb::remote_work_ipi(unsigned this_cpu, unsigned to_cpu,
                     void (*f)(unsigned, void *), void *data, bool wait = true)
{
  if (to_cpu == this_cpu)
    {
      f(this_cpu, data);
      return true;
    }

  if (!Cpu::online(to_cpu))
    return false;

  auto guard = lock_guard(_remote_call_lock);

  _remote_work_ipi_func      = f;
  _remote_work_ipi_func_data = data;
  _remote_work_ipi_done      = 0;

  Ipi::send(Ipi::Debug, this_cpu, to_cpu);

  if (wait)
    while (!*(volatile unsigned long *)&_remote_work_ipi_done)
      Proc::pause();

  _remote_work_ipi_func = 0;

  return true;
}
