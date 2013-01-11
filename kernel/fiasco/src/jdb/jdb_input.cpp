INTERFACE:

#include "jdb_module.h"
#include "types.h"
#include "l4_types.h"

class Jdb_input
{
};

class Task;
class Space;
class Kobject;

class Jdb_input_task_addr
{
public:
  static char     first_char;
  static char     first_char_have_task;
private:
  static Kobject *_task;
  static Space   *_space;
  static Address  _addr;
public:
  typedef struct
    {
      char str[32];
      Jdb_module::Gotkey *gotkey;
    } Symbol;
  static Symbol   symbol;
};


IMPLEMENTATION:

#include <cctype>
#include <cstring>
#include <cstdio>
#include "kernel_console.h"
#include "keycodes.h"
#include "jdb.h"
#include "jdb_kobject.h"
#include "jdb_symbol.h"
#include "simpleio.h"
#include "task.h"


char     Jdb_input_task_addr::first_char;
char     Jdb_input_task_addr::first_char_have_task;
Kobject *Jdb_input_task_addr::_task;
Space   *Jdb_input_task_addr::_space;
Address  Jdb_input_task_addr::_addr;
Jdb_input_task_addr::Symbol Jdb_input_task_addr::symbol;


static
void
Jdb_input_task_addr::gotkey_complete_symbol(char *&str, int maxlen, int c)
{ (void)str; (void)maxlen;
  if (c == KEY_TAB)
    printf("XXX\n"); //Jdb_symbol::complete_symbol(str, maxlen, task);
}

PUBLIC static
Task *
Jdb_input_task_addr::task()
{ return Kobject::dcast<Task *>(_task); }

PUBLIC static
Space *
Jdb_input_task_addr::space()
{ return _space; }

PUBLIC static
Address
Jdb_input_task_addr::addr()
{ return _addr; }

PUBLIC static
Jdb_module::Action_code
Jdb_input_task_addr::action(void *&args, char const *&fmt, int &next_char)
{
  if (args == &first_char)
    {
      // initialize
      // so _task is only valid if it is explicitly set
      _task         = 0;
      _space        = Jdb::get_current_space();
      _addr         = (Address)-1;
      symbol.str[0] = '\0';
      symbol.gotkey = gotkey_complete_symbol;
    }

  if (args == &first_char_have_task)
    first_char = first_char_have_task;

  if (args == &first_char && first_char == 't')
    {
      args = &_task;
      fmt  = strcmp(fmt, " addr=%C") ? " task=%q" : " \b\b\b\b\b\btask=%q";
      return Jdb_module::EXTRA_INPUT;
    }
  else if (args == &_task)
    {
      _space = Kobject::dcast<Task *>(_task);

      if (_task && !space())
	{
	  _task = 0;
	  puts(" invalid task");
	  return Jdb_module::ERROR;
	}

      args = &first_char_have_task;
      fmt  = " addr=%C";
      return Jdb_module::EXTRA_INPUT;
    }
  else if (first_char == 'p')
    {
      _task = 0;
      _space = 0;
      putstr(" [phys-mem]");

      args = &first_char_have_task;
      fmt  = " addr=%C";
      return Jdb_module::EXTRA_INPUT;
    }
  else if (args == &first_char || args == &first_char_have_task)
    {
      if (first_char == 's')
	{
	  args = &symbol;
	  fmt  = strcmp(fmt, " addr=%C") ? " symbol=%32S"
					 : " \b\b\b\b\b\bsymbol=%32S";
	  return Jdb_module::EXTRA_INPUT;
	}
      else if (isxdigit(first_char))
	{
	  next_char = first_char;
	  args = &_addr;
	  fmt  = L4_ADDR_INPUT_FMT;
	  return Jdb_module::EXTRA_INPUT_WITH_NEXTCHAR;
	}
      else if (first_char != KEY_RETURN && first_char != ' ')
	{
	  fmt = "%C";
	  return Jdb_module::EXTRA_INPUT;
	}
    }
  else if (args == &symbol)
    {
#if 0
      // XXX: Fix symbols
      Address sym_addr;
      bool instr = strlen(symbol.str)>=sizeof(symbol.str)-1;
      if ((sym_addr = Jdb_symbol::match_symbol_to_addr(symbol.str,
						       instr, task)))
	_addr = sym_addr;
      else
	{
	  _addr = (Address)-1;
	  puts(" not found");
	  return Jdb_module::ERROR;
	}
#endif
    }

  return Jdb_module::NOTHING;
}

PUBLIC static
int
Jdb_input::get_mword(Mword *mword, int digits, int base, int first_char = 0)
{
  Mword val=0;

  for(int digit=0; digit<digits; )
    {
      int c, v;

      if (first_char)
	{
	  c = v = first_char;
	  first_char = 0;
	}
      else
	c = v = Kconsole::console()->getchar();
      switch(c) 
	{
	case 'a': case 'b': case 'c':
	case 'd': case 'e': case 'f':
	  if (base == 16)
	    v -= 'a' - '9' - 1;
	  else
	    continue;
	case '0': case '1': case '2':
	case '3': case '4': case '5':
	case '6': case '7': case '8':
	case '9':
	  val = (val * base) + (v - '0');
	  digit++;
	  putchar(c);
	  break;
	case KEY_BACKSPACE:
	  if (digit) 
	    {
	      putstr("\b \b");
	      digit--;
	      val /= base;
	    }
	  break;
	case 'x':
	  // If last digit was 0, delete it. This makes it easier to cut 
	  // 'n paste hex values like 0x12345678 into the serial terminal
	  if (base == 16 && digit && ((val & 0x10) == 0))
	    {
	      putstr("\b \b");
	      digit--;
	      val /= base;
	    }
	  break;
	case ' ':
	case KEY_RETURN: 
	  if (digit) 
	    { 
	      *mword = val;
	      return true; 
	    }
	  *mword = 0; 
	  return false; 
	case KEY_ESC:
	  return 0;
	}
    }
  *mword = val;
  return 1;
}

