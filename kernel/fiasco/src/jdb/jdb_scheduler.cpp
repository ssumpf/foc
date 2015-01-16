IMPLEMENTATION:

#include <climits>
#include <cstring>
#include <cstdio>

#include "scheduler.h"
#include "jdb.h"
#include "jdb_core.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "jdb_kobject.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "simpleio.h"
#include "static_init.h"

class Jdb_cpu : public Jdb_kobject_handler
{
public:
  Jdb_cpu() FIASCO_INIT;
};

IMPLEMENT
Jdb_cpu::Jdb_cpu()
  : Jdb_kobject_handler(Scheduler::static_kobj_type)
{
  Jdb_kobject::module()->register_handler(this);
}

PUBLIC
bool
Jdb_cpu::show_kobject(Kobject_common *, int )
{
  return true;
}

PUBLIC
char const *
Jdb_cpu::kobject_type() const
{
  return JDB_ANSI_COLOR(blue) "Sched" JDB_ANSI_COLOR(default);
}

static Jdb_cpu jdb_cpu INIT_PRIORITY(JDB_MODULE_INIT_PRIO);
