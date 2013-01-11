IMPLEMENTATION:

#include <cstdio>

#include "config.h"
#include "jdb.h"
#include "jdb_kobject.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "jdb_table.h"
#include "kernel_console.h"
#include "kmem.h"
#include "keycodes.h"
#include "space.h"
#include "task.h"
#include "thread_object.h"
#include "static_init.h"
#include "types.h"

class Jdb_ptab_m : public Jdb_module, public Jdb_kobject_handler
{
public:
  Jdb_ptab_m() FIASCO_INIT;
private:
  Address task;
  static char first_char;
  bool show_kobject(Kobject_common *, int) { return false; }
};

class Jdb_ptab : public Jdb_table
{
private:
  Address base;
  Address virt_base;
  int _level;
  Space *_task;
  unsigned entries;
  unsigned char cur_pt_level;
  char dump_raw;

  static unsigned max_pt_level;

  static unsigned entry_valid(Mword entry, unsigned level);
  static unsigned entry_is_pt_ptr(Mword entry, unsigned level,
                                  unsigned *entries, unsigned *next_level);
  static Address entry_phys(Mword entry, unsigned level);

  void print_entry(Mword entry, unsigned level);
  void print_head(Mword entry);
};

char Jdb_ptab_m::first_char;

typedef Mword My_pte;			// shoud be replaced by
					// arch-dependent type

PUBLIC
Jdb_ptab::Jdb_ptab(void *pt_base = 0, Space *task = 0,
                   unsigned char pt_level = 0, unsigned entries = 0,
                   Address virt_base = 0, int level = 0)
: base((Address)pt_base), virt_base(virt_base), _level(level),
  _task(task), entries(entries), cur_pt_level(pt_level), dump_raw(0)
{
  if (!pt_level && entries == 0)
    this->entries = 1UL << Ptab::Level<Pdir::Traits,0>::Traits::Size;
}

PUBLIC
unsigned
Jdb_ptab::col_width(unsigned column) const
{
  if (column == 0)
    return Jdb_screen::Col_head_size;
  else
    return Jdb_screen::Mword_size_bmode;
}

PUBLIC
unsigned long
Jdb_ptab::cols() const
{
  return Jdb_screen::cols();
}


// available from the jdb_dump module
int jdb_dump_addr_task(Address addr, Space *task, int level)
  __attribute__((weak));


PUBLIC
void
Jdb_ptab::draw_entry(unsigned long row, unsigned long col)
{
  if (col==0)
    print_head(virt(row, 1));
  else
    print_entry(*(My_pte*)(virt(row,col)), cur_pt_level);
}

PRIVATE
Address
Jdb_ptab::virt(unsigned long row, unsigned long col)
{
  Mword e = (col-1) + (row * (cols()-1));
  return base + e * sizeof(Mword);
}

IMPLEMENT
void
Jdb_ptab::print_head(Mword entry)
{
  printf(L4_PTR_FMT, entry);
}

PUBLIC
bool
Jdb_ptab_m::handle_key(Kobject_common *o, int code)
{
  if (code != 'p')
    return false;

  Space *t = Kobject::dcast<Task*>(o);
  if (!t)
    {
      Thread *th = Kobject::dcast<Thread_object*>(o);
      if (!th || !th->space())
	return false;

      t = th->space();
    }

  Jdb_ptab pt_view(static_cast<Mem_space*>(t)->dir(), t, 0, 0, 0, 1);
  pt_view.show(0,0);

  return true;
}

PUBLIC
unsigned 
Jdb_ptab::key_pressed(int c, unsigned long &row, unsigned long &col)
{
  switch (c)
    {
    default:
      return Nothing;

    case KEY_CURSOR_HOME: // return to previous or go home
      if (_level == 0)
	return Nothing;
      return Back;

    case ' ':
      dump_raw ^= 1;
      return Redraw;

    case KEY_RETURN:	// goto ptab/address under cursor
      if (_level<=7)
	{
	  My_pte pt_entry = *(My_pte*)virt(row,col);
	  if (!entry_valid(pt_entry, cur_pt_level))
	    break;

	  Address pd_virt = (Address)
	    Mem_layout::phys_to_pmem(entry_phys(pt_entry, cur_pt_level));

	  unsigned next_level, entries;

	  if (cur_pt_level < max_pt_level
	      && entry_is_pt_ptr(pt_entry, cur_pt_level, &entries, &next_level))
	    {
	      Jdb_ptab pt_view((void *)pd_virt, _task, next_level, entries,
		               disp_virt(row,col), _level+1);
	      if (!pt_view.show(0,1))
		return Exit;
	      return Redraw;
	    }
	  else if (jdb_dump_addr_task != 0)
	    {
	      if (!jdb_dump_addr_task(disp_virt(row,col), _task, _level+1))
		return Exit;
	      return Redraw;
	    }
	}
      break;
    }

  return Handled;
}

PUBLIC
Jdb_module::Action_code
Jdb_ptab_m::action(int cmd, void *&args, char const *&fmt, int &next_char)
{
  if (cmd == 0)
    {
      if (args == &first_char)
	{
	  if (first_char != KEY_RETURN && first_char != ' ')
	    {
	      fmt       = "%q";
	      args      = &task;
	      next_char = first_char;
	      return EXTRA_INPUT_WITH_NEXTCHAR;
	    }
	  else
	    {
	      task = 0; //Jdb::get_current_task();
	    }
	}
      else if (args == &task)
	{
#if 0
	  if (!Jdb::is_valid_task(task))
	    {
	      puts(" invalid task");
	      return NOTHING;
	    }
#endif
	}
      else
	return NOTHING;

      Space *s;
      if (task)
        {
          s = Kobject::dcast<Task*>(reinterpret_cast<Kobject*>(task));
          if (!s)
	    return Jdb_module::NOTHING;
        }
      else
        s = Kernel_task::kernel_task();

      void *ptab_base;
      if (!(ptab_base = ((void*)static_cast<Mem_space*>(s)->dir())))
	return Jdb_module::NOTHING;

      Jdb::clear_screen();
      Jdb_ptab pt_view(ptab_base, s);
      pt_view.show(0,1);

    }

  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *
Jdb_ptab_m::cmds() const
{
  static Cmd cs[] =
    {
	{ 0, "p", "ptab", "%C",
	  "p[<taskno>]\tshow pagetable of current/given task",
	  &first_char },
    };
  return cs;
}

PUBLIC
int
Jdb_ptab_m::num_cmds() const
{
  return 1;
}

IMPLEMENT
Jdb_ptab_m::Jdb_ptab_m()
  : Jdb_module("INFO"), Jdb_kobject_handler(0)
{
  Jdb_kobject::module()->register_handler(this);
}

static Jdb_ptab_m jdb_ptab_m INIT_PRIORITY(JDB_MODULE_INIT_PRIO);
