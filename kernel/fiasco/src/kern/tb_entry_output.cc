#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "config.h"
#include "jdb_symbol.h"
#include "jdb_tbuf_output.h"
#include "jdb_util.h"
#include "kobject_dbg.h"
#include "static_init.h"
#include "tb_entry.h"
#include "thread.h"

static __attribute__((format(printf, 3, 4)))
void
my_snprintf(char *&buf, int &maxlen, const char *format, ...)
{
  int len;
  va_list list;

  if (maxlen<=0)
    return;

  va_start(list, format);
  len = vsnprintf(buf, maxlen, format, list);
  va_end(list);

  if (len<0 || len>=maxlen)
    len = maxlen-1;

  buf    += len;
  maxlen -= len;
}


// timeout => x.x{
static
void
format_timeout(Mword us, char *&buf, int &maxlen)
{
  if (us >= 1000000000)		// =>100s
    my_snprintf(buf, maxlen, ">99s");
  else if (us >= 10000000)	// >=10s
    my_snprintf(buf, maxlen, "%lds", us/1000000);
  else if (us >= 1000000)	// >=1s
    my_snprintf(buf, maxlen, "%ld.%lds", us/1000000, (us%1000000)/100000);
  else if (us >= 10000)		// 10ms
    my_snprintf(buf, maxlen, "%ldm", us/1000);
  else if (us >= 1000)		// 1ms
    my_snprintf(buf, maxlen, "%ld.%ldm", us/1000, (us%1000)/100);
  else
    my_snprintf(buf, maxlen, "%ld%c", us, Config::char_micro);
}

template< typename T >
static
char const *
get_ipc_type(T e)
{
  switch (e->ipc_type())
    {
    case L4_obj_ref::Ipc_call:           return "call";
    case L4_obj_ref::Ipc_call_ipc:       return "call ipc";
    case L4_obj_ref::Ipc_send:           return "send";
    case L4_obj_ref::Ipc_recv:           return "recv";
    case L4_obj_ref::Ipc_send_and_wait:  return "snd wait";
    case L4_obj_ref::Ipc_reply_and_wait: return "repl";
    case L4_obj_ref::Ipc_wait:           return "wait";
    default:                             return "unk ipc";
    }
}

static char const * const __tag_interpreter_strings_l4re[] = {
    "ds", // 0
    "name",
    "parent",
    "goos",
    "ma",
    "rm", // 5
    "ev",
  };
static char const * const __tag_interpreter_strings_fiasco[] = {
    "Kirq",      // -1
    "Kpf",
    "Kpreempt",
    "Ksysexc",
    "Kexc",      // -5
    "Ksigma",
    0,
    "Kiopf",
    "Kcapfault",
    0,           // -10
    "Ktask",
    "Kthread",
    "Klog",
    "Ksched",
    "Kfactory",  // -15
    "Kvm",
    0,
    0,
    0,
    "Ksem",      // -20
    "Kmeta",
  };

static
char const *
tag_to_string(L4_msg_tag const &tag)
{
  if (0x4000 <= tag.proto() && tag.proto() <= 0x4006)
    return __tag_interpreter_strings_l4re[tag.proto() - 0x4000];
  if (-21L <= tag.proto() && tag.proto() <= -1)
    return __tag_interpreter_strings_fiasco[-tag.proto() - 1];
  return 0;
}

static
void
tag_interpreter_snprintf(char *&buf, int &maxlen, L4_msg_tag const &tag)
{
  int len;
  char const *s;

  if (maxlen<=0)
    return;

  s = tag_to_string(tag);
  if (s)
    len = snprintf(buf, maxlen, "%s%04lx", s, tag.raw() & 0xffff);
  else
    len = snprintf(buf, maxlen, L4_PTR_FMT, tag.raw());

  if (len<0 || len>=maxlen)
    len = maxlen-1;

  buf    += len;
  maxlen -= len;
}

class Tb_entry_ipc_fmt : public Tb_entry_formatter
{
public:
  unsigned print(Tb_entry const *, int, char *) const { return 0; }
  Group_order has_partner(Tb_entry const *) const
  { return Tb_entry::Group_order::first(); }
  Group_order is_pair(Tb_entry const *e, Tb_entry const *n) const
  {
    if (static_cast<Tb_entry_ipc_res const *>(n)->pair_event() == e->number())
      return Group_order::last();
    else
      return Group_order::none();
  }
  Mword partner(Tb_entry const *) const { return 0; }
};

class Tb_entry_ipc_res_fmt : public Tb_entry_formatter
{
public:
  unsigned print(Tb_entry const *, int, char *) const { return 0; }
  Group_order has_partner(Tb_entry const *) const
  { return Tb_entry::Group_order::last(); }
  Group_order is_pair(Tb_entry const *e, Tb_entry const *n) const
  {
    if (static_cast<Tb_entry_ipc_res const *>(e)->pair_event() == n->number())
      return Group_order::first();
    else
      return Group_order::none();
  }
  Mword partner(Tb_entry const *e) const { return static_cast<Tb_entry_ipc_res const *>(e)->pair_event(); }
};


// ipc / irq / shortcut success
static
unsigned
formatter_ipc(Tb_entry *tb, const char *tidstr, unsigned tidlen,
	      char *buf, int maxlen)
{
  Tb_entry_ipc *e = static_cast<Tb_entry_ipc*>(tb);
  unsigned char type = e->ipc_type();
  if (!type)
    type = L4_obj_ref::Ipc_call_ipc;

  // ipc operation / shortcut succeeded/failed
  const char *m = get_ipc_type(e);

  my_snprintf(buf, maxlen, "%s: ", (e->type()==Tbuf_ipc) ? "ipc" : "sc ");  
  my_snprintf(buf, maxlen, "%s%-*s %s->",
      /*e->dst().next_period() ? "[NP] " :*/ "", tidlen, tidstr, m);

  // print destination id
  if (e->dst().special())
    my_snprintf(buf, maxlen, "[C:INV] DID=%lx", e->dbg_id());
  else
    my_snprintf(buf, maxlen, "[C:%lx] DID=%lx", e->dst().raw(), e->dbg_id());

  my_snprintf(buf, maxlen, " L=%lx", e->label());

  // print mwords if have send part
  if (type & L4_obj_ref::Ipc_send)
    {
      my_snprintf(buf, maxlen, " [");

      tag_interpreter_snprintf(buf, maxlen, e->tag());

      my_snprintf(buf, maxlen, "] (" L4_PTR_FMT "," L4_PTR_FMT ")",
	                       e->dword(0), e->dword(1));
    }

  my_snprintf(buf, maxlen, " TO=");
  L4_timeout_pair to = e->timeout();
  if (type & L4_obj_ref::Ipc_send)
    {
      if (to.snd.is_absolute())
	{
	  // absolute send timeout
	  Unsigned64 end = 0; // FIXME: to.snd.microsecs_abs (e->kclock());
	  format_timeout((Mword)(end > e->kclock() ? end-e->kclock() : 0),
			 buf, maxlen);
	}
      else
	{
	  // relative send timeout
	  if (to.snd.is_never())
	    my_snprintf(buf, maxlen, "INF");
	  else if (to.snd.is_zero())
	    my_snprintf(buf, maxlen, "0");
	  else
            format_timeout((Mword)to.snd.microsecs_rel(0), buf, maxlen);
	}
    }
  if (type & L4_obj_ref::Ipc_send
      && type & L4_obj_ref::Ipc_recv)
    my_snprintf(buf, maxlen, "/");
  if (type & L4_obj_ref::Ipc_recv)
    {
      if (to.rcv.is_absolute())
	{
	  // absolute receive timeout
	  Unsigned64 end = 0; // FIXME: to.rcv.microsecs_abs (e->kclock());
	  format_timeout((Mword)(end > e->kclock() ? end-e->kclock() : 0),
			 buf, maxlen);
	}
      else
	{
	  // relative receive timeout
	  if (to.rcv.is_never())
	    my_snprintf(buf, maxlen, "INF");
	  else if (to.rcv.is_zero())
	    my_snprintf(buf, maxlen, "0");
	  else
            format_timeout((Mword)to.rcv.microsecs_rel(0), buf, maxlen);
	}
    }

  return maxlen;
}

// result of ipc
static
unsigned
formatter_ipc_res(Tb_entry *tb, const char *tidstr, unsigned tidlen,
		  char *buf, int maxlen)
{
  Tb_entry_ipc_res *e = static_cast<Tb_entry_ipc_res*>(tb);
  L4_error error;
  if (e->tag().has_error())
    error = e->result();
  else
    error = L4_error::None;
  const char *m = "answ"; //get_ipc_type(e);

  my_snprintf(buf, maxlen, "     %s%-*s %s [%08lx] L=%lx err=%lx (%s) (%lx,%lx) ",
			    e->is_np() ? "[np] " : "",
			    tidlen, tidstr, m, e->tag().raw(), e->from(),
                            error.raw(), error.str_error(), e->dword(0),
			    e->dword(1));

  return maxlen;
}


// pagefault
static
unsigned
formatter_pf(Tb_entry *tb, const char *tidstr, unsigned tidlen,
	     char *buf, int maxlen)
{
  Tb_entry_pf *e = static_cast<Tb_entry_pf*>(tb);
  char ip_buf[32];

  snprintf(ip_buf, sizeof(ip_buf), L4_PTR_FMT, e->ip());
  my_snprintf(buf, maxlen, "pf:  %-*s pfa=" L4_PTR_FMT " ip=%s (%c%c) spc=%p",
      tidlen, tidstr, e->pfa(), e->ip() ? ip_buf : "unknown",
      !PF::is_read_error(e->error()) ? (e->error() & 4 ? 'w' : 'W')
                                     : (e->error() & 4 ? 'r' : 'R'),
      !PF::is_translation_error(e->error()) ? 'p' : '-',
      e->space());

  return maxlen;
}

// pagefault
unsigned
Tb_entry_pf::print(int maxlen, char *buf) const
{
  char ip_buf[32];

  snprintf(ip_buf, sizeof(ip_buf), L4_PTR_FMT, ip());
  my_snprintf(buf, maxlen, "pfa=" L4_PTR_FMT " ip=%s (%c%c) spc=%p",
      pfa(), ip() ? ip_buf : "unknown",
      !PF::is_read_error(error()) ? (error() & 4 ? 'w' : 'W')
                                  : (error() & 4 ? 'r' : 'R'),
      !PF::is_translation_error(error()) ? 'p' : '-',
      space());

  return maxlen;
}

// kernel event (enter_kdebug("*..."))
static
unsigned
formatter_ke(Tb_entry *tb, const char *tidstr, unsigned tidlen,
	     char *buf, int maxlen)
{
  Tb_entry_ke *e = static_cast<Tb_entry_ke*>(tb);
  char ip_buf[32];

  snprintf(ip_buf, sizeof(ip_buf), " @ " L4_PTR_FMT, e->ip());
  my_snprintf(buf, maxlen, "ke:  %-*s \"%s\"%s",
      tidlen, tidstr, e->msg(), e->ip() ? ip_buf : "");

  return maxlen;
}

// kernel event (enter_kdebug_verb("*+...", val1, val2, val3))
static
unsigned
formatter_ke_reg(Tb_entry *tb, const char *tidstr, unsigned tidlen,
		 char *buf, int maxlen)
{
  Tb_entry_ke_reg *e = static_cast<Tb_entry_ke_reg*>(tb);

  char ip_buf[32];
  snprintf(ip_buf, sizeof(ip_buf), " @ " L4_PTR_FMT, e->ip());
  my_snprintf(buf, maxlen, "ke:  %-*s \"%s\" "
      L4_PTR_FMT " " L4_PTR_FMT " " L4_PTR_FMT "%s",
      tidlen, tidstr, e->msg(), e->v[0], e->v[1], e->v[2],
      e->ip() ? ip_buf : "");

  return maxlen;
}


// breakpoint
static
unsigned
formatter_bp(Tb_entry *tb, const char *tidstr, unsigned tidlen,
	     char *buf, int maxlen)
{
  Tb_entry_bp *e = static_cast<Tb_entry_bp*>(tb);

  my_snprintf(buf, maxlen, "b%c:  %-*s @ " L4_PTR_FMT " ",
      "iwpa"[e->mode() & 3], tidlen, tidstr, e->ip());
  switch (e->mode() & 3)
    {
    case 1:
    case 3:
      switch (e->len())
	{
     	case 1:
	  my_snprintf(buf, maxlen, "[" L4_PTR_FMT "]=%02lx", 
	              e->addr(), e->value());
	  break;
	case 2:
	  my_snprintf(buf, maxlen, "[" L4_PTR_FMT "]=%04lx", 
	      	      e->addr(), e->value());
	  break;
	case 4:
	  my_snprintf(buf, maxlen, "[" L4_PTR_FMT "]=" L4_PTR_FMT,
	      	      e->addr(), e->value());
	  break;
	}
      break;
    case 2:
      my_snprintf(buf, maxlen, "[" L4_PTR_FMT "]", e->addr());
      break;
    }

  return maxlen;
}


// trap
unsigned
Tb_entry_trap::print(int maxlen, char *buf) const
{
  if (!cs())
    my_snprintf(buf, maxlen, "#%02x: err=%08x @ " L4_PTR_FMT,
		trapno(), error(), ip());
  else
    my_snprintf(buf, maxlen,
	        trapno() == 14
		  ? "#%02x: err=%04x @ " L4_PTR_FMT
		    " cs=%04x sp=" L4_PTR_FMT " cr2=" L4_PTR_FMT
		  : "#%02x: err=%04x @ " L4_PTR_FMT
		    " cs=%04x sp=" L4_PTR_FMT " eax=" L4_PTR_FMT,
	        trapno(),
		error(), ip(), cs(), sp(),
		trapno() == 14 ? cr2() : eax());

  return maxlen;
}

// kernel event log binary data
static
unsigned
formatter_ke_bin(Tb_entry *tb, const char *tidstr, unsigned tidlen,
	         char *buf, int maxlen)
{
  Tb_entry_ke_bin *e = static_cast<Tb_entry_ke_bin*>(tb);
  char ip_buf[32];

  snprintf(ip_buf, sizeof(ip_buf), " @ " L4_PTR_FMT, e->ip());
  my_snprintf(buf, maxlen, "ke:  %-*s BINDATA %s",
              tidlen, tidstr, e->ip() ? ip_buf : "");

  return maxlen;
}

static Tb_entry_ipc_fmt const _ipc_fmt;
static Tb_entry_ipc_res_fmt const _ipc_res_fmt;

// register all available format functions
static FIASCO_INIT
void
init_formatters()
{
  Jdb_tbuf_output::register_ff(Tbuf_pf, formatter_pf);
  Jdb_tbuf_output::register_ff(Tbuf_ipc, formatter_ipc);
  Tb_entry_formatter::set_fixed(Tbuf_ipc, &_ipc_fmt);
  Jdb_tbuf_output::register_ff(Tbuf_ipc_res, formatter_ipc_res);
  Tb_entry_formatter::set_fixed(Tbuf_ipc_res, &_ipc_res_fmt);
  Jdb_tbuf_output::register_ff(Tbuf_ke, formatter_ke);
  Jdb_tbuf_output::register_ff(Tbuf_ke_reg, formatter_ke_reg);
  Jdb_tbuf_output::register_ff(Tbuf_breakpoint, formatter_bp);
  Jdb_tbuf_output::register_ff(Tbuf_ke_bin, formatter_ke_bin);
}

STATIC_INITIALIZER(init_formatters);
