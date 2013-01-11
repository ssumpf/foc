INTERFACE [debug]:

EXTENSION class Thread
{
protected:
  struct Log_thread_exregs
  {
    Mword       id, ip, sp, op;
  };
  static unsigned fmt_exregs(Tb_entry *, int, char *) asm ("__fmt_thread_exregs");
};

//--------------------------------------------------------------------------
IMPLEMENTATION [debug]:

#include <cstdio>
#include "config.h"
#include "kmem.h"
#include "mem_layout.h"
#include "simpleio.h"


//---------------------------------------------------------------------------
IMPLEMENTATION [debug]:

IMPLEMENT
unsigned
Thread::fmt_exregs(Tb_entry *e, int max, char *buf)
{
  Log_thread_exregs *l = e->payload<Log_thread_exregs>();
  return snprintf(buf, max, "D=%lx ip=%lx sp=%lx op=%s%s%s",
                  l->id, l->ip, l->sp,
                  l->op & Exr_cancel ? "Cancel" : "",
                  ((l->op & (Exr_cancel |
                             Exr_trigger_exception))
                   == (Exr_cancel |
                       Exr_trigger_exception))
                   ? ","
                   : ((l->op & (Exr_cancel |
                                Exr_trigger_exception))
                      == 0 ? "0" : "") ,
                  l->op & Exr_trigger_exception ? "TrExc" : "");
}
