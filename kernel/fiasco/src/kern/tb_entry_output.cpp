IMPLEMENTATION:

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "config.h"
#include "jdb_symbol.h"
#include "jdb_tbuf_output.h"
#include "jdb_util.h"
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

IMPLEMENTATION:

#include "kobject_dbg.h"

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

// pagefault result
static
unsigned
formatter_pf_res(Tb_entry *tb, const char *tidstr, unsigned tidlen,
		 char *buf, int maxlen)
{
  Tb_entry_pf_res *e = static_cast<Tb_entry_pf_res*>(tb);

  // e->ret contains only an error code
  // e->err contains only up to 3 dwords
  my_snprintf(buf, maxlen, "     %-*s pfa=" L4_PTR_FMT " dope=%02lx (%s%s) "
		"err=%04lx (%s%s)",
	      tidlen, tidstr, e->pfa(),
	      e->ret().raw(), e->ret().error() ? "L4_IPC_" : "",
	      e->ret().str_error(),
	      e->err().raw(), e->err().error() ? "L4_IPC_" : "",
	      e->err().str_error());

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
      tidlen, tidstr, e->msg(), e->val1(), e->val2(), e->val3(), 
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

// context switch
static
unsigned
formatter_ctx_switch(Tb_entry *tb, const char *tidstr, unsigned tidlen,
		     char *buf, int maxlen)
{
  char symstr[24], spcstr[16] = "";
  Tb_entry_ctx_sw *e = static_cast<Tb_entry_ctx_sw*>(tb);

  Context   *sctx    = 0;
  Mword sctxid = ~0UL;
  Mword dst;
  Mword dst_orig;

  sctx = e->from_sched()->context();
  sctxid = static_cast<Thread*>(sctx)->dbg_id();

  dst = static_cast<Thread const *>(e->dst())->dbg_id();
  dst_orig = static_cast<Thread const *>(e->dst_orig())->dbg_id();

  Address addr       = e->kernel_ip();

  if (!Jdb_symbol::match_addr_to_symbol_fuzzy(&addr, 0 /*kernel*/,
					      symstr, sizeof(symstr)))
    snprintf(symstr, sizeof(symstr), L4_PTR_FMT, e->kernel_ip());

  my_snprintf(buf, maxlen, "     %-*s%s '%02lx",
      tidlen, tidstr, spcstr, e->from_prio());
  if (sctx != e->ctx())
    my_snprintf(buf, maxlen, "(%lx)", sctxid);

  my_snprintf(buf, maxlen, " ==> %lx ", dst);

  if (dst != dst_orig || e->lock_cnt())
    my_snprintf(buf, maxlen, "(");

  if (dst != dst_orig)
    my_snprintf(buf, maxlen, "want %lx",
	dst_orig);

  if (dst != dst_orig && e->lock_cnt())
    my_snprintf(buf, maxlen, " ");

  if (e->lock_cnt())
    my_snprintf(buf, maxlen, "lck %ld", e->lock_cnt());

  if (dst != dst_orig || e->lock_cnt())
    my_snprintf(buf, maxlen, ") ");

  my_snprintf(buf, maxlen, " krnl %s", symstr);

  return maxlen;
}


// trap
static
unsigned
formatter_trap(Tb_entry *tb, const char *tidstr, unsigned tidlen,
	       char *buf, int maxlen)
{
  Tb_entry_trap *e = static_cast<Tb_entry_trap*>(tb);

  if (!e->cs())
    my_snprintf(buf, maxlen, "#%02x: %-*s err=%08x @ " L4_PTR_FMT,
		e->trapno(), tidlen, tidstr, e->error(), e->ip());
  else
    my_snprintf(buf, maxlen,
	        e->trapno() == 14
		  ? "#%02x: %-*s err=%04x @ " L4_PTR_FMT
		    " cs=%04x sp=" L4_PTR_FMT " cr2=" L4_PTR_FMT
		  : "#%02x: %-*s err=%04x @ " L4_PTR_FMT
		    " cs=%04x sp=" L4_PTR_FMT " eax=" L4_PTR_FMT,
	        e->trapno(),
		tidlen, tidstr, e->error(), e->ip(), e->cs(), e->sp(),
		e->trapno() == 14 ? e->cr2() : e->eax());

  return maxlen;
}

// sched
static
unsigned
formatter_sched(Tb_entry *tb, const char *tidstr, unsigned tidlen,
		char *buf, int maxlen)
{
  Tb_entry_sched *e = static_cast<Tb_entry_sched*>(tb);
  Thread const *_t = static_cast<Thread const *>(e->owner());
  Mword t = ~0UL;
  if (Jdb_util::is_mapped(_t))
    t = _t->dbg_id();


  my_snprintf(buf, maxlen, 
            "%-*s (ts %s) owner:%lx id:%2x, prio:%2x, left:%6ld/%-6lu",
               tidlen, tidstr,
               e->mode() == 0 ? "save" :
               e->mode() == 1 ? "load" :
               e->mode() == 2 ? "invl" : "????",
               t,
               e->id(), e->prio(), e->left(), e->quantum());

  return maxlen;
}

// preemption
static
unsigned
formatter_preemption(Tb_entry *tb, const char *tidstr, unsigned tidlen,
		     char *buf, int maxlen)
{
  Tb_entry_preemption *e = static_cast<Tb_entry_preemption*>(tb);
  Mword t = e->preempter();

  my_snprintf(buf, maxlen,
	     "pre: %-*s sent to %lx",
	     tidlen, tidstr, t);

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

STATIC_INITIALIZER(init_formatters);

// register all available format functions
static FIASCO_INIT
void
init_formatters()
{
  Jdb_tbuf_output::register_ff(Tbuf_pf, formatter_pf);
  Jdb_tbuf_output::register_ff(Tbuf_pf_res, formatter_pf_res);
  Jdb_tbuf_output::register_ff(Tbuf_ipc, formatter_ipc);
  Jdb_tbuf_output::register_ff(Tbuf_ipc_res, formatter_ipc_res);
  Jdb_tbuf_output::register_ff(Tbuf_ke, formatter_ke);
  Jdb_tbuf_output::register_ff(Tbuf_ke_reg, formatter_ke_reg);
  Jdb_tbuf_output::register_ff(Tbuf_shortcut_succeeded, formatter_ipc);
  Jdb_tbuf_output::register_ff(Tbuf_context_switch, formatter_ctx_switch);
  Jdb_tbuf_output::register_ff(Tbuf_breakpoint, formatter_bp);
  Jdb_tbuf_output::register_ff(Tbuf_trap, formatter_trap);
  Jdb_tbuf_output::register_ff(Tbuf_sched, formatter_sched);
  Jdb_tbuf_output::register_ff(Tbuf_preemption, formatter_preemption);
  Jdb_tbuf_output::register_ff(Tbuf_ke_bin, formatter_ke_bin);
}
