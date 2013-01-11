INTERFACE:

class Jdb_list
{
public:
  virtual char const *get_mode_str() const { return "[std mode]"; }
  virtual void next_mode() {}
  virtual void next_sort() {}
  virtual void *get_head() const = 0;
  virtual int show_item(char *buffer, int max, void *item) const = 0;
  virtual char const *show_head() const = 0;
  virtual int seek(int cnt, void **item) = 0;
  virtual bool enter_item(void * /*item*/) const { return true; }
  virtual void *follow_link(void *a) { return a; }
  virtual bool handle_key(void * /*item*/, int /*keycode*/) { return false; }
  virtual void *parent(void * /*item*/) { return 0; }
  virtual void *get_valid(void *a) { return a; }

private:
  void *_start, *_last;
  void *_current;

};


// ---------------------------------------------------------------------------
IMPLEMENTATION:

#include <climits>
#include <cstring>
#include <cstdio>

#include "jdb.h"
#include "jdb_core.h"
#include "jdb_screen.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "simpleio.h"
#include <minmax.h>



PUBLIC
Jdb_list::Jdb_list()
  : _start(0), _current(0)
{}

// set _t_start element of list
PUBLIC
void
Jdb_list::set_start(void *start)
{
  _start = start;
}

// _t_start-- if possible
PUBLIC inline
bool
Jdb_list::line_back()
{ return seek(-1, &_start); }

// _t_start++ if possible
PUBLIC inline
bool
Jdb_list::line_forw()
{
  if (seek(1, &_last))
    return seek(1, &_start);
  else
    return false;
}
#if 0
  Thread *t = _t_start;
  iter(+Jdb_screen::height()-2, &_t_start);
  iter(-Jdb_screen::height()+3, &_t_start);
  return t != _t_start;
}
#endif

// _t_start -= 24 if possible
PUBLIC
bool
Jdb_list::page_back()
{ return seek(-Jdb_screen::height()+2, &_start); }

// _t_start += 24 if possible
PUBLIC
bool
Jdb_list::page_forw()
{
  int fwd = seek(Jdb_screen::height()-2, &_last);
  if (fwd)
    return seek(fwd, &_start);
  return false;
}

#if 0
  Thread *t = _t_start;
  iter(+Jdb_screen::height()*2-5, &_t_start);
  iter(-Jdb_screen::height()  +3, &_t_start);
  return t != _t_start;
}
#endif

// _t_start = first element of list
PUBLIC
bool
Jdb_list::goto_home()
{ return seek(-99999, &_start); }

// _t_start = last element of list
PUBLIC
bool
Jdb_list::goto_end()
{ return seek(99999, &_start); }
#if 0
  Thread *t = _t_start;
  iter(+9999, &_t_start);
  iter(-Jdb_screen::height()+2, &_t_start);
  return t != _t_start;
}
#endif

// search index of t_search starting from _t_start
PUBLIC
int
Jdb_list::lookup_in_visible_area(void *search)
{
  unsigned i;
  void *t;

  for (i=0, t = _start; i < Jdb_screen::height()-3; ++i)
    {
      if (t == search)
	return i;

      seek(1, &t);
    }

  return -1;
}

// get y'th element of thread list starting from _t_start
PUBLIC
void *
Jdb_list::index(int y)
{
  void *t = _start;

  seek(y, &t);
  return t;
}


PUBLIC
void
Jdb_list::show_line(void *i)
{
  static char buffer[256];
  Kconsole::console()->getchar_chance();
  int pos = 0;
  void *p = i;
  while ((p = parent(p)))
    {
      buffer[pos] = ' ';
      ++pos;
    }

  pos += show_item(buffer + pos, sizeof(buffer) - pos, i);
  if (i)
    printf("%.*s\033[K\n", min((int)Jdb_screen::width(), pos), buffer);
}

// show complete page using show callback function
PUBLIC
int
Jdb_list::page_show()
{
  void *t = _start;
  unsigned i = 0;
  for (i = 0; i < Jdb_screen::height()-3; ++i)
    {
      if (!t)
	break;
      else
	_last = t;

      show_line(t);

      if (!seek(1,&t))
	return i;
    }

  return i - 1;
}

// show complete list using show callback function
PUBLIC
int
Jdb_list::complete_show()
{
  void *t = _start;
  int i = 0;
  for (i = 0; ; ++i, seek(1, &t))
    {
      if (!t)
	break;

      show_line(t);
    }

  return i;
}

#if 0
PUBLIC
Jdb_module::Action_code
Jdb_thread_list::action(int cmd, void *&argbuf, char const *&fmt, int &)
{
  static char const *const cpu_fmt = " cpu=%i\n";
  static char const *const nfmt = "\n";
  if (cmd == 0)
    {
      if (fmt != cpu_fmt && fmt != nfmt)
	{
	  if (subcmd == 'c')
	    {
	      argbuf = &cpu;
	      fmt = cpu_fmt;
	    }
	  else
	    fmt = nfmt;
	  return EXTRA_INPUT;
	}

      Thread *t = Jdb::get_current_active();
      switch (subcmd)
	{
	case 'r': cpu = 0; list_threads(t, 'r'); break;
	case 'p': list_threads(t, 'p'); break;
	case 'c': 
		  if (Cpu::online(cpu))
		    list_threads(Jdb::get_thread(cpu), 'r');
		  else
		    printf("\nCPU %u is not online!\n", cpu);
		  cpu = 0;
		  break;
	case 't': Jdb::execute_command("lt"); break; // other module
	}
    }
  else if (cmd == 1)
    {
      Console *gzip = Kconsole::console()->find_console(Console::GZIP);
      if (gzip)
	{
	  Thread *t = Jdb::get_current_active();
	  gzip->state(gzip->state() | Console::OUTENABLED);
	  long_output = 1;
	  Jdb_thread_list::init('p', t);
	  Jdb_thread_list::set_start(t);
	  Jdb_thread_list::goto_home();
	  Jdb_thread_list::complete_show(list_threads_show_thread);
	  long_output = 0;
	  gzip->state(gzip->state() & ~Console::OUTENABLED);
	}
      else
	puts(" gzip module not available");
    }

  return NOTHING;
}
#endif


PUBLIC
void
Jdb_list::show_header()
{
  Jdb::cursor();
  printf("%.*s\033[K\n", Jdb_screen::width(), show_head());
}


PUBLIC
void
Jdb_list::do_list()
{
  int y, y_max;
  void *t;

  if (!_start)
    _start = get_head();

  if (!_current)
    _current = _start;

  Jdb::clear_screen();
  show_header();

  if (!_start)
    {
      printf("[EMPTY]\n");
      return;
    }


  for (;;)
    {
      // set y to position of t_current in current displayed list
      y = lookup_in_visible_area(_current);
      if (y == -1)
	{
	  _start = _current;
	  y = 0;
	}

      for (bool resync=false; !resync;)
	{
	  Jdb::cursor(2, 1);
	  y_max = page_show();

	  // clear rest of screen (if where less than 24 lines)
	  for (unsigned i = y_max; i < Jdb_screen::height()-3; ++i)
            putstr("\033[K\n");

	  Jdb::printf_statline("Objs",
			       "<Space>=mode <Tab>=link <CR>=select",
			       "%-15s", get_mode_str());

	  // key event loop
	  for (bool redraw=false; !redraw; )
	    {
	      Jdb::cursor(y+2, 1);
	      switch (int c=Jdb_core::getchar())
		{
		case KEY_CURSOR_UP:
		case 'k':
		  if (y > 0)
		    y--;
		  else
		    redraw = line_back();
		  break;
		case KEY_CURSOR_DOWN:
		case 'j':
		  if (y < y_max)
		    y++;
		  else
		    redraw = line_forw();
		  break;
		case KEY_PAGE_UP:
		case 'K':
		  if (!(redraw = page_back()))
		    y = 0;
		  break;
		case KEY_PAGE_DOWN:
		case 'J':
		  if (!(redraw = page_forw()))
		    y = y_max;
		  break;
		case KEY_CURSOR_HOME:
		case 'H':
		  redraw = goto_home();
		  y = 0;
		  break;
		case KEY_CURSOR_END:
		case 'L':
		  redraw = goto_end();
		  y = y_max;
		  break;
		case 's': // switch sort
		  _current = index(y);
		  next_sort();
		  redraw = true;
		  resync = true;
		  break;
		case ' ': // switch mode
                  _current = index(y);
		  next_mode();
		  _current = get_valid(_current);
                  _start   = get_valid(_start);
		  redraw = true;
		  resync = true;
		  break;
		case KEY_TAB: // go to associated object
		  _current = index(y);
		  t = follow_link(_current);
		  if (t != _current)
		    {
		      _current = t;
		      redraw = true;
		      resync = true;
		    }
		  break;
		case KEY_RETURN:
		  _current = index(y);
		  if (!enter_item(_current))
		    return;
		  show_header();
		  redraw = 1;
		  break;
		case KEY_ESC:
		  Jdb::abort_command();
		  return;
		default:
		  _current = index(y);
		  if (!handle_key(_current, c) && Jdb::is_toplevel_cmd(c))
		    return;

		  show_header();
		  redraw = 1;
		  break;
		}
	    }
	}
    }
}

