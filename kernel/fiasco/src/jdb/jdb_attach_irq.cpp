IMPLEMENTATION:

#include <cstdio>

#include "irq_chip.h"
#include "irq.h"
#include "irq_mgr.h"
#include "jdb_module.h"
#include "kernel_console.h"
#include "static_init.h"
#include "thread.h"
#include "types.h"


//===================
// Std JDB modules
//===================

/**
 * 'IRQ' module.
 *
 * This module handles the 'R' command that
 * provides IRQ attachment and listing functions.
 */
class Jdb_attach_irq : public Jdb_module
{
public:
  Jdb_attach_irq() FIASCO_INIT;
private:
  static char     subcmd;
};

char     Jdb_attach_irq::subcmd;
static Jdb_attach_irq jdb_attach_irq INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

IMPLEMENT
Jdb_attach_irq::Jdb_attach_irq()
  : Jdb_module("INFO")
{}

PUBLIC
Jdb_module::Action_code
Jdb_attach_irq::action( int cmd, void *&args, char const *&, int & )
{
  if (cmd)
    return NOTHING;

  if ((char*)args == &subcmd)
    {
      switch (subcmd)
        {
        case 'l': // list
            {
              Irq_base *r;
              putchar('\n');
	      unsigned n = Irq_mgr::mgr->nr_irqs();
              for (unsigned i = 0; i < n; ++i)
                {
                  r = static_cast<Irq*>(Irq_mgr::mgr->irq(i));
                  if (!r)
                    continue;
                  printf("IRQ %02x/%02d\n", i, i);
                }
              return NOTHING;
            }
        }
    }
  return NOTHING;
}

PUBLIC
int
Jdb_attach_irq::num_cmds() const
{
  return 1;
}

PUBLIC
Jdb_module::Cmd const *
Jdb_attach_irq::cmds() const
{
  static Cmd cs[] =
    {   { 0, "R", "irq", " [l]ist/[a]ttach: %c",
	   "R{l}\tlist IRQ threads", &subcmd }
    };

  return cs;
}


IMPLEMENTATION:

#include "jdb_kobject.h"
#include "irq.h"

class Jdb_kobject_irq : public Jdb_kobject_handler
{
};


/* This macro does kind of hacky magic, It uses heuristics to compare
 * If a C++ object object has a desired type. Therefore the vtable pointer
 * (*z) of an object is compared to a desired vtable pointer (with some fuzz).
 * The fuzz is necessary because there is usually a prefix data structure
 * in each vtable and the size depends on the compiler.
 * We use a range from x to x + 6 * wordsize.
 *
 * It is generally uncritical if this macro delivers a false negative. In
 * such a case the JDB may deliver less information to the user. However, it
 * critical to have a false positive, because JDB would probably crash.
 */
#define FIASCO_JDB_CMP_VTABLE(n, o) \
  extern char n[]; \
  char const *const *z = reinterpret_cast<char const* const*>(o); \
  return (*z >= n && *z <= n + 6 * sizeof(Mword)) ? (o) : 0


PUBLIC static
Irq_sender *
Jdb_kobject_irq::dcast_h(Irq_sender *i)
{
  FIASCO_JDB_CMP_VTABLE(_ZTV10Irq_sender, i);
}

PUBLIC static
Irq_muxer *
Jdb_kobject_irq::dcast_h(Irq_muxer *i)
{
  FIASCO_JDB_CMP_VTABLE(_ZTV9Irq_muxer, i);
}

PUBLIC template<typename T>
static
T
Jdb_kobject_irq::dcast(Kobject_common *o)
{
  Irq *i = Kobject::dcast<Irq*>(o);
  if (!i)
    return 0;

  return dcast_h(static_cast<T>(i));
}

PUBLIC inline
Jdb_kobject_irq::Jdb_kobject_irq()
  : Jdb_kobject_handler(Irq::static_kobj_type)
{
  Jdb_kobject::module()->register_handler(this);
}

PUBLIC
char const *
Jdb_kobject_irq::kobject_type() const
{
  return JDB_ANSI_COLOR(white) "IRQ" JDB_ANSI_COLOR(default);
}


PUBLIC
bool
Jdb_kobject_irq::handle_key(Kobject_common *o, int key)
{
  (void)o; (void)key;
  return false;
}



PUBLIC
Kobject_common *
Jdb_kobject_irq::follow_link(Kobject_common *o)
{
  Irq_sender *t = Jdb_kobject_irq::dcast<Irq_sender*>(o);
  Kobject_common *k = t ? Kobject::from_dbg(Kobject_dbg::pointer_to_obj(t->owner())) : 0;
  return k ? k : o;
}

PUBLIC
bool
Jdb_kobject_irq::show_kobject(Kobject_common *, int)
{ return true; }

PUBLIC
void
Jdb_kobject_irq::show_kobject_short(String_buffer *buf, Kobject_common *o)
{
  Irq *i = Kobject::dcast<Irq*>(o);
  Kobject_common *w = follow_link(o);
  Irq_sender *t = Jdb_kobject_irq::dcast<Irq_sender*>(o);

  buf->printf(" I=%3lx %s L=%lx T=%lx F=%x Q=%d",
              i->pin(), i->chip()->chip_type(), i->obj_id(),
              w != o ?  w->dbg_info()->dbg_id() : 0,
              (unsigned)i->flags(),
              t ? t->queued() : -1);
}

static
bool
filter_irqs(Kobject_common const *o)
{ return Kobject::dcast<Irq const *>(o); }

static Jdb_kobject_list::Mode INIT_PRIORITY(JDB_MODULE_INIT_PRIO) tnt("[IRQs]", filter_irqs);

static Jdb_kobject_irq jdb_kobject_irq INIT_PRIORITY(JDB_MODULE_INIT_PRIO);
