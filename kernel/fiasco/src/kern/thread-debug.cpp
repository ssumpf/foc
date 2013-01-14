INTERFACE [debug]:

#include "tb_entry.h"

EXTENSION class Thread
{
protected:
  struct Log_thread_exregs : public Tb_entry
  {
    Mword       id, ip, sp, op;
    unsigned print(int, char *) const;
  };
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
Thread::Log_thread_exregs::print(int max, char *buf) const
{
  return snprintf(buf, max, "D=%lx ip=%lx sp=%lx op=%s%s%s",
                  id, ip, sp,
                  op & Exr_cancel ? "Cancel" : "",
                  ((op & (Exr_cancel | Exr_trigger_exception))
                   == (Exr_cancel | Exr_trigger_exception))
                   ? ","
                   : ((op & (Exr_cancel | Exr_trigger_exception))
                      == 0 ? "0" : "") ,
                  op & Exr_trigger_exception ? "TrExc" : "");
}
