IMPLEMENTATION:

#include <cstdio>

#include "jdb.h"
#include "jdb_input.h"
#include "jdb_screen.h"
#include "kernel_console.h"
#include "kobject.h"
#include "keycodes.h"
#include "mapdb.h"
#include "mapdb_i.h"
#include "map_util.h"
#include "simpleio.h"
#include "static_init.h"
#include "task.h"
#include "jdb_kobject.h"
#include "jdb_kobject_names.h"

class Jdb_mapdb : public Jdb_module
{
  friend class Jdb_kobject_mapdb_hdl;
public:
  Jdb_mapdb() FIASCO_INIT;
private:
  static Mword pagenum;
  static char  subcmd;
};

char  Jdb_mapdb::subcmd;
Mword Jdb_mapdb::pagenum;

static
const char*
size_str (Mword size)
{
  static char scratchbuf[6];
  unsigned mult = 0;
  while (size >= 1024)
    {
      size >>= 10;
      mult++;
    }
  snprintf (scratchbuf, 6, "%u%c", unsigned(size), "BKMGTPX"[mult]);
  return scratchbuf;
}

static
bool
Jdb_mapdb::show_tree(Treemap* pages, Page_number address,
		     unsigned &screenline, unsigned indent = 1)
{
  Page_number   page = address >> pages->_page_shift;
  Physframe*    f = pages->frame(page);
  Mapping_tree* t = f->tree.get();
  unsigned      i;
  int           c;

  if (! t)
    {
      printf(" no mapping tree registered for frame number 0x%x\033[K\n",
	     (unsigned) page.value());
      screenline++;
      return true;
    }

  printf(" mapping tree for %s-page " L4_PTR_FMT " of task %p - header at "
	 L4_PTR_FMT "\033[K\n",
	 size_str (1UL << pages->_page_shift),
	 pages->vaddr(t->mappings()).value(), t->mappings()[0].space(), (Address)t);
#ifdef NDEBUG
  // If NDEBUG is active, t->_empty_count is undefined
  printf(" header info: "
	 "entries used: %u  free: --  total: %u  lock=%u\033[K\n",
	 t->_count, t->number_of_entries(),
	 f->lock.test());

  if (t->_count > t->number_of_entries())
    {
      printf("\033[K\n"
	     "\033[K\n"
	     "  seems to be a wrong tree ! ...exiting");
      // clear rest of page
      for (i=6; i<Jdb_screen::height(); i++)
	printf("\033[K\n");
      return false;
    }
#else
  printf(" header info: "
	 "entries used: %u  free: %u  total: %u  lock=%u\033[K\n",
	 t->_count, t->_empty_count, t->number_of_entries(),
	 f->lock.test());

  if (unsigned (t->_count) + t->_empty_count > t->number_of_entries())
    {
      printf("\033[K\n"
	     "\033[K\n"
	     "  seems to be a wrong tree ! ...exiting");
      // clear rest of page
      for (i=6; i<Jdb_screen::height(); i++)
	printf("\033[K\n");
      return false;
    }
#endif

  Mapping* m = t->mappings();

  screenline += 2;

  for (i=0; i < t->_count; i++, m++)
    {
      Kconsole::console()->getchar_chance();

      if (m->depth() == Mapping::Depth_submap)
	printf("%*u: %lx  subtree@" L4_PTR_FMT,
	       indent + m->parent()->depth() > 10
	         ? 0 : indent + m->parent()->depth(),
	       i+1, (Address) m->data(), (Mword) m->submap());
      else
	{
	  printf("%*u: %lx  va=" L4_PTR_FMT "  task=%lx  depth=",
		 indent + m->depth() > 10 ? 0 : indent + m->depth(),
		 i+1, (Address) m->data(),
		 pages->vaddr(m).value(),
                 Kobject_dbg::pointer_to_id(m->space()));

	  if (m->depth() == Mapping::Depth_root)
	    printf("root");
	  else if (m->depth() == Mapping::Depth_empty)
	    printf("empty");
	  else if (m->depth() == Mapping::Depth_end)
	    printf("end");
	  else
	    printf("%lu", static_cast<unsigned long>(m->depth()));
	}

      puts("\033[K");
      screenline++;

      if (screenline >= (m->depth() == Mapping::Depth_submap
			 ? Jdb_screen::height() - 3
			 : Jdb_screen::height()))
	{
	  printf(" any key for next page or <ESC>");
	  Jdb::cursor(screenline, 33);
	  c = Jdb_core::getchar();
	  printf("\r\033[K");
	  if (c == KEY_ESC)
	    return false;
	  screenline = 3;
	  Jdb::cursor(3, 1);
	}

      if (m->depth() == Mapping::Depth_submap)
	{
	  if (! Jdb_mapdb::show_tree(m->submap(),
				     address.offset(Page_count::create(1UL << pages->_page_shift)),
				     screenline, indent + m->parent()->depth()))
	    return false;
	}
    }

  return true;
}

static
Address
Jdb_mapdb::end_address (Mapdb* mapdb)
{
  return mapdb->_treemap->end_addr().value();
}

static
void
Jdb_mapdb::show (Page_number page, char which_mapdb)
{
  unsigned     j;
  int          c;

  Jdb::clear_screen();

  for (;;)
    {
      Mapdb* mapdb;
      const char* type;
      Mword page_shift;
      Page_count super_inc;

      switch (which_mapdb)
	{
	case 'm':
	  type = "Phys frame";
	  mapdb = mapdb_mem.get();
	  page_shift = 0; //Config::PAGE_SHIFT;
	  super_inc = Page_count::create(Config::SUPERPAGE_SIZE / Config::PAGE_SIZE);
	  break;
#ifdef CONFIG_IO_PROT
	case 'i':
	  type = "I/O port";
	  mapdb = mapdb_io.get();
	  page_shift = 0;
	  super_inc = Page_count::create(0x100);
	  break;
#endif
	default:
	  return;
	}

      if (! mapdb->valid_address (page << page_shift))
	page = Page_number::create(0);

      Jdb::cursor();
      printf ("%s " L4_PTR_FMT "\033[K\n\033[K\n",
	      type, page.value() << page_shift);

      j = 3;

      if (! Jdb_mapdb::show_tree (mapdb->_treemap,
				  (page << page_shift)
				    - mapdb->_treemap->_page_offset,
				  j))
	return;

      for (; j<Jdb_screen::height(); j++)
	puts("\033[K");

      static char prompt[] = "mapdb[m]";
      prompt[6] = which_mapdb;

      Jdb::printf_statline(prompt,
			   "n=next p=previous N=nextsuper P=prevsuper", "_");

      for (bool redraw=false; !redraw; )
	{
	  Jdb::cursor(Jdb_screen::height(), 10);
	  switch (c = Jdb_core::getchar())
	    {
	    case 'n':
	    case KEY_CURSOR_DOWN:
	      if (! mapdb->valid_address(++page << page_shift))
                page = Page_number::create(0);
	      redraw = true;
	      break;
	    case 'p':
	    case KEY_CURSOR_UP:
	      if (! mapdb->valid_address(--page << page_shift))
                page = Page_number::create(end_address (mapdb) - 1) >> page_shift;
	      redraw = true;
	      break;
	    case 'N':
	    case KEY_PAGE_DOWN:
	      page = (page + super_inc).trunc(super_inc);
	      if (! mapdb->valid_address(page << page_shift))
                page = Page_number::create(0);
	      redraw = true;
	      break;
	    case 'P':
	    case KEY_PAGE_UP:
	      page = (page - super_inc).trunc(super_inc);
	      if (! mapdb->valid_address(page << page_shift))
                page = Page_number::create(end_address (mapdb) - 1) >> page_shift;
	      redraw = true;
	      break;
	    case ' ':
	      if (which_mapdb == 'm')
#ifdef CONFIG_IO_PROT
		which_mapdb = 'i';
	      else if (which_mapdb == 'i')
#endif
		which_mapdb = 'm';
	      redraw = true;
              break;
	    case KEY_ESC:
	      Jdb::abort_command();
	      return;
	    default:
	      if (Jdb::is_toplevel_cmd(c))
		return;
	    }
	}
    }
}

PUBLIC
Jdb_module::Action_code
Jdb_mapdb::action(int cmd, void *&args, char const *&fmt, int &next_char)
{
  static char which_mapdb = 'm';

  if (cmd == 1)
    {
      dump_all_cap_trees();
      return NOTHING;
    }

  if (cmd != 0)
    return NOTHING;

  if (args == (void*) &subcmd)
    {
      switch (subcmd)
	{
	default:
	  return NOTHING;

	case '\r':
	case ' ':
	  goto doit;

	case '0' ... '9':
	case 'a' ... 'f':
	case 'A' ... 'F':
	  which_mapdb = 'm';
	  fmt = " frame: " L4_FRAME_INPUT_FMT;
	  args = &pagenum;
	  next_char = subcmd;
	  return EXTRA_INPUT_WITH_NEXTCHAR;

	case 'm':
	  fmt = " frame: " L4_FRAME_INPUT_FMT;
	  break;

#ifdef CONFIG_IO_PROT
	case 'i':
	  fmt = " port: " L4_FRAME_INPUT_FMT;
	  break;
#endif

	case 'o':
	  fmt = " object: %x";
	  break;
	}

      which_mapdb = subcmd;
      args = &pagenum;
      return EXTRA_INPUT;
    }

  else if (args != (void*) &pagenum)
    return NOTHING;

 doit:
  if (which_mapdb == 'o')
    Jdb_mapdb::show_simple_tree((Kobject_common*)pagenum);
  else
    show(Page_number::create(pagenum), which_mapdb);
  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *
Jdb_mapdb::cmds() const
{
  static Cmd cs[] =
    {
	{ 0, "m", "mapdb", "%c",
	  "m[it]<addr>\tshow [I/O,task] mapping database starting at address",
	  &subcmd },
        { 1, "", "dumpmapdbobjs", "",
          "dumpmapdbobjs\tDump complete object mapping database", 0 },
    };
  return cs;
}

PUBLIC
int
Jdb_mapdb::num_cmds() const
{
  return 2;
}

IMPLEMENT
Jdb_mapdb::Jdb_mapdb()
  : Jdb_module("INFO")
{}

static Jdb_mapdb jdb_mapdb INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

// --------------------------------------------------------------------------
// Handler for kobject list

class Jdb_kobject_mapdb_hdl : public Jdb_kobject_handler
{
public:
  Jdb_kobject_mapdb_hdl() : Jdb_kobject_handler(0) {}
  virtual bool show_kobject(Kobject_common *, int) { return true; }
  virtual ~Jdb_kobject_mapdb_hdl() {}
};

PUBLIC static FIASCO_INIT
void
Jdb_kobject_mapdb_hdl::init()
{
  static Jdb_kobject_mapdb_hdl hdl;
  Jdb_kobject::module()->register_handler(&hdl);
}

PUBLIC
bool
Jdb_kobject_mapdb_hdl::handle_key(Kobject_common *o, int keycode)
{
  if (keycode == 'm')
    {
      Jdb_mapdb::show_simple_tree(o);
      Jdb::getchar();
      return true;
    }
  else
    return false;
}



STATIC_INITIALIZE(Jdb_kobject_mapdb_hdl);

#if 0 // keep this for reanimation
static
void
Jdb_mapdb::dump_all_cap_trees()
{
  printf("========= OBJECT DUMP BEGIN ===================\n");
  Kobject *f = static_cast<Kobject*>(Kobject::_jdb_head.get_unused());
  for (; f; f = static_cast<Kobject*>(f->_next))
    {
      char s[130];

      Jdb_kobject::obj_description(s, sizeof(s), true, f);
      s[sizeof(s) - 1] = 0;
      printf("%s", s);

      Mapping_tree *t = f->tree.get();

      if (!t)
        {
          printf("\n");
	  continue;
        }

      Mapping *m = t->mappings();

      printf(" intask=");
      for (int i = 0; i < t->_count; i++, m++)
	{
          if (m->depth() == Mapping::Depth_submap)
            printf("%s[subtree]", i ? "," : "");
          else
            printf("%s[%lx:%d]",
                   i ? "," : "", Kobject::pointer_to_id(m->space()),
                   m->depth());
	}
      printf("\n");

      if (m->depth() == Mapping::Depth_submap)
        {
          printf("not good, submap in simple mapping tree\n");
        }
    }
  printf("========= OBJECT DUMP END ===================\n");
}
#endif

// --------------------------------------------------------------------------
IMPLEMENTATION:

#include "dbg_page_info.h"

static
bool
Jdb_mapdb::show_simple_tree(Kobject_common *f, unsigned indent = 1)
{
  (void)indent;
  (void)f;

  unsigned      screenline = 0;
  int           c;

  puts(Jdb_screen::Line);
  if (!f || f->map_root()->_root.empty())
    {
      printf(" no mapping tree registered for frame number 0x%lx\033[K\n",
	     (unsigned long) f);
      screenline++;
      puts(Jdb_screen::Line);
      return true;
    }

  printf(" mapping tree for object D:%lx (%p) ref_cnt=%ld\n",
         f->dbg_info()->dbg_id(), f, f->map_root()->_cnt);

  screenline += 2;

  for (Obj::Mapping::List::Iterator m = f->map_root()->_root.begin();
       m != f->map_root()->_root.end(); ++m)
    {
      Kconsole::console()->getchar_chance();

      Obj::Entry *e = static_cast<Obj::Entry*>(*m);
      Dbg_page_info *pi = Dbg_page_info::table()[Virt_addr(e)];

      Mword space_id = ~0UL;
      Address cap_idx = ((Address)e % Config::PAGE_SIZE) / sizeof(Obj::Entry);

      if (pi)
	{
	  space_id = static_cast<Task*>(pi->info<Obj_space::Dbg_info>()->s)->dbg_info()->dbg_id();
	  cap_idx += pi->info<Obj_space::Dbg_info>()->offset;
	}

      printf("  " L4_PTR_FMT "[C:%lx]: space=D:%lx rights=%x flags=%lx obj=%p",
	     (Address)*m, cap_idx, space_id, (unsigned)e->rights(), e->_flags,
	     e->obj());

      puts("\033[K");
      screenline++;

      if (screenline >= Jdb_screen::height())
	{
	  printf(" any key for next page or <ESC>");
	  Jdb::cursor(screenline, 33);
	  c = Jdb_core::getchar();
	  printf("\r\033[K");
	  if (c == KEY_ESC)
	    return false;
	  screenline = 3;
	  Jdb::cursor(3, 1);
	}
    }

  puts(Jdb_screen::Line);



  return true;
}

static
void
Jdb_mapdb::dump_all_cap_trees()
{
  printf("========= OBJECT DUMP BEGIN ===================\n");
  printf("========= OBJECT DUMP END ===================\n");
}


