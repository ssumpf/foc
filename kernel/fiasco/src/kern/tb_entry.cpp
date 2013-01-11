INTERFACE:

#include "initcalls.h"
#include "l4_error.h"

enum {
  Tbuf_unused             = 0,
  Tbuf_pf                 = 1,
  Tbuf_ipc                = 2,
  Tbuf_ipc_res            = 3,
  Tbuf_ipc_trace          = 4,
  Tbuf_ke                 = 5,
  Tbuf_ke_reg             = 6,
  Tbuf_unmap              = 7,
  Tbuf_shortcut_failed    = 8,
  Tbuf_shortcut_succeeded = 9,
  Tbuf_context_switch     = 10,
  Tbuf_exregs             = 11,
  Tbuf_breakpoint         = 12,
  Tbuf_trap               = 13,
  Tbuf_pf_res             = 14,
  Tbuf_sched              = 15,
  Tbuf_preemption         = 16,
  Tbuf_ke_bin             = 17,
  Tbuf_dynentries         = 18,
  Tbuf_max                = 0x80,
  Tbuf_hidden             = 0x80,
};

#include "l4_types.h"

class Tb_entry;

typedef unsigned Tb_entry_formatter(Tb_entry *e, int max, char *buf);

struct Tb_log_table_entry
{
  char const *name;
  unsigned char *patch;
  Tb_entry_formatter *fmt;
};

extern Tb_log_table_entry _log_table[];
extern Tb_log_table_entry _log_table_end;

class Context;
class Space;
class Sched_context;
class Syscall_frame;
class Trap_state;

class Tb_entry_base
{
protected:
  Mword		_number;	///< event number
  Address	_ip;		///< instruction pointer
  Context const *_ctx;		///< Context
  Unsigned64	_tsc;		///< time stamp counter
  Unsigned32	_pmc1;		///< performance counter value 1
  Unsigned32	_pmc2;		///< performance counter value 2
  Unsigned32	_kclock;	///< lower 32 bits of kernel clock
  Unsigned8	_type;		///< type of entry
  Unsigned8     _cpu;           ///< CPU

  static Mword (*rdcnt1)();
  static Mword (*rdcnt2)();
} __attribute__((packed));

class Tb_entry : public Tb_entry_base
{
public:
  template<typename T>
  class Payload_traits
  {
  public:
    enum
    {
      Align  = __alignof__(T), //(sizeof(T) > sizeof(Mword)) ? sizeof(Mword) : sizeof(T),
      Offset = sizeof(Tb_entry_base),
      Payload_offset = ((Offset + Align -1) & ~(Align-1)) - Offset,
      Total = Offset + Payload_offset + sizeof(T),
    };

    template<int I>
    struct TTI
    {};

    static void tb_entry_size_mismatch(TTI<false> const &) {}
    static void payload_too_big(TTI<false> const &) {}
    static void payload_too_big()
    {
      tb_entry_size_mismatch(TTI<(sizeof(Tb_entry) != Tb_entry_size)>());
      payload_too_big(TTI<((int)Total > (int)Tb_entry_size)>());
    }

  };

private:
  template< typename T >
  T *addr_of_payload() const
  {
    Payload_traits<T>::payload_too_big();

    Address p = reinterpret_cast<Address>(_payload);
    return reinterpret_cast<T*>(p + Payload_traits<T>::Payload_offset);
  }

public:
  char _payload[Tb_entry_size-sizeof(Tb_entry_base)];

  template<typename T>
  T *payload() { return addr_of_payload<T>(); }
  
  template<typename T>
  T const *payload() const { return addr_of_payload<T>(); }


} __attribute__((__aligned__(8))) ;

/** logged ipc. */
class Tb_entry_ipc : public Tb_entry
{
private:
  struct Payload
  {
    L4_msg_tag	_tag;           ///< message tag
    Mword	_dword[2];	///< first two message words
    L4_obj_ref	_dst;		///< destination id
    Mword       _dbg_id;
    Mword       _label;
    L4_timeout_pair _timeout;	///< timeout
  };
};

/** logged ipc result. */
class Tb_entry_ipc_res : public Tb_entry
{
private:
  struct Payload
  {
    L4_msg_tag	_tag;		///< message tag
    Mword	_dword[2];	///< first two dwords
    L4_error	_result;	///< result
    Mword	_from;		///< receive descriptor
    Mword	_pair_event;	///< referred event
    Unsigned8	_have_snd;	///< ipc had send part
    Unsigned8	_is_np;		///< next period bit set
  };
};

/** logged ipc for user level tracing with Vampir. */
class Tb_entry_ipc_trace : public Tb_entry
{
private:
  struct Payload
  {
    Unsigned64	_snd_tsc;	///< entry tsc
    L4_msg_tag	_result;	///< result
    L4_obj_ref	_snd_dst;	///< send destination
    Mword	_rcv_dst;	///< rcv partner
    Unsigned8	_snd_desc;
    Unsigned8	_rcv_desc;
  };
};

#if 0
/** logged short-cut ipc failed. */
class Tb_entry_ipc_sfl : public Tb_entry_base
{
private:
  Global_id	_from;	///< short ipc rcv descriptor
  L4_timeout_pair	_timeout;	///< ipc timeout
  Global_id	_dst;		///< partner
  Unsigned8	_is_irq, _snd_lst, _dst_ok, _dst_lck, _preempt;
};
#endif

/** logged pagefault. */
class Tb_entry_pf : public Tb_entry
{
private:
  struct Payload
  {
    Address	_pfa;		///< pagefault address
    Mword	_error;		///< pagefault error code
    Space	*_space;
  };
};

/** pagefault result. */
class Tb_entry_pf_res : public Tb_entry
{
private:
  struct Payload
  {
    Address	_pfa;
    L4_error	_err;
    L4_error	_ret;
  };
};


/** logged kernel event. */
class Tb_entry_ke : public Tb_entry
{
  enum { MSG_POINTER_PAYLOAD_OFFSET
           = 5 - ((sizeof(Tb_entry_base) + 2 + 3) % sizeof(Mword)) };
};

/** logged breakpoint. */
class Tb_entry_bp : public Tb_entry
{
private:
  struct Payload
  {
    Address	_address;	///< breakpoint address
    int		_len;		///< breakpoint length
    Mword	_value;		///< value at address
    int		_mode;		///< breakpoint mode
  } __attribute__((packed));
};

/** logged context switch. */
class Tb_entry_ctx_sw : public Tb_entry
{
private:
  struct Payload
  {
    Context const *_dst;		///< switcher target
    Context const *_dst_orig;
    Address	_kernel_ip;
    Mword	_lock_cnt;
    Space	*_from_space;
    Sched_context *_from_sched;
    Mword	_from_prio;
  } __attribute__((packed));
};

/** logged scheduling event. */
class Tb_entry_sched : public Tb_entry
{
private:
  struct Payload
  {
    unsigned short _mode;
    Context const *_owner;
    unsigned short _id;
    unsigned short _prio;
    signed long	 _left;
    unsigned long  _quantum;
  } __attribute__((packed));
};


/** logged preemption */
class Tb_entry_preemption : public Tb_entry
{
private:
  struct Payload
  {
    Mword	 _preempter;
  };
};


/** logged binary kernel event. */
class Tb_entry_ke_bin : public Tb_entry
{
public:
  enum { SIZE = 30 };
};



IMPLEMENTATION:

#include <cstring>
#include <cstdarg>

#include "entry_frame.h"
#include "globals.h"
#include "kip.h"
#include "static_init.h"
#include "trap_state.h"


PROTECTED static Mword Tb_entry_base::dummy_read_pmc() { return 0; }

Mword (*Tb_entry_base::rdcnt1)() = dummy_read_pmc;
Mword (*Tb_entry_base::rdcnt2)() = dummy_read_pmc;


PUBLIC static
void
Tb_entry::set_rdcnt(int num, Mword (*f)())
{
  if (!f)
    f = dummy_read_pmc;

  switch (num)
    {
    case 0: rdcnt1 = f; break;
    case 1: rdcnt2 = f; break;
    }
}

PUBLIC inline
void
Tb_entry::clear()
{ _type = Tbuf_unused; }

PUBLIC inline NEEDS["kip.h", "globals.h"]
void
Tb_entry::set_global(char type, Context const *ctx, Address ip)
{
  _type   = type;
  _ctx    = ctx;
  _ip     = ip;
  _kclock = (Unsigned32)Kip::k()->clock;
  _cpu    = current_cpu();
}

PUBLIC inline
void
Tb_entry::hide()
{ _type |= Tbuf_hidden; }

PUBLIC inline
void
Tb_entry::unhide()
{ _type &= ~Tbuf_hidden; }

PUBLIC inline
Address
Tb_entry::ip() const
{ return _ip; }

PUBLIC inline
Context const *
Tb_entry::ctx() const
{ return _ctx; }

PUBLIC inline
Unsigned8
Tb_entry::type() const
{ return _type & (Tbuf_max-1); }

PUBLIC inline
int
Tb_entry::hidden() const
{ return _type & Tbuf_hidden; }

PUBLIC inline
Mword
Tb_entry::number() const
{ return _number; }

PUBLIC inline
void
Tb_entry::number(Mword number)
{ _number = number; }

PUBLIC inline
void
Tb_entry::rdpmc1()
{ _pmc1 = rdcnt1(); }

PUBLIC inline
void
Tb_entry::rdpmc2()
{ _pmc2 = rdcnt2(); }

PUBLIC inline
Unsigned32
Tb_entry::kclock() const
{ return _kclock; }

PUBLIC inline
Unsigned8
Tb_entry::cpu() const
{ return _cpu; }

PUBLIC inline
Unsigned64
Tb_entry::tsc() const
{ return _tsc; }

PUBLIC inline
Unsigned32
Tb_entry::pmc1() const
{ return _pmc1; }

PUBLIC inline
Unsigned32
Tb_entry::pmc2() const
{ return _pmc2; }


PUBLIC inline NEEDS ["entry_frame.h"]
void
Tb_entry_ipc::set(Context const *ctx, Mword ip, Syscall_frame *ipc_regs, Utcb *utcb,
		  Mword dbg_id, Unsigned64 left)
{
  set_global(Tbuf_ipc, ctx, ip);
  Payload *p = payload<Payload>();
  p->_dst       = ipc_regs->ref();
  p->_label     = ipc_regs->from_spec();


  p->_dbg_id = dbg_id;

  p->_timeout   = ipc_regs->timeout();
  p->_tag       = ipc_regs->tag();
  if (ipc_regs->next_period())
    {
      p->_dword[0]  = (Unsigned32)(left & 0xffffffff);
      p->_dword[1]  = (Unsigned32)(left >> 32);
    }
  else
    {
      // hint for gcc
      register Mword tmp0 = utcb->values[0];
      register Mword tmp1 = utcb->values[1];
      p->_dword[0]  = tmp0;
      p->_dword[1]  = tmp1;
    }
}

PUBLIC inline
void
Tb_entry_ipc::set_sc(Context const *ctx, Mword ip, Syscall_frame *ipc_regs,
                     Utcb *utcb, Unsigned64 left)
{
  set_global(Tbuf_shortcut_succeeded, ctx, ip);
  Payload *p = payload<Payload>();
  p->_dst       = ipc_regs->ref();
  p->_timeout   = ipc_regs->timeout();
  if (ipc_regs->next_period())
    {
      p->_dword[0]  = (Unsigned32)(left & 0xffffffff);
      p->_dword[1]  = (Unsigned32)(left >> 32);
    }
  else
    {
      // hint for gcc
      register Mword tmp0 = utcb->values[0];
      register Mword tmp1 = utcb->values[1];
      p->_dword[0]  = tmp0;
      p->_dword[1]  = tmp1;
    }
}


PUBLIC inline
Mword
Tb_entry_ipc::ipc_type() const
{ return payload<Payload>()->_dst.op(); }

PUBLIC inline
Mword
Tb_entry_ipc::dbg_id() const
{ return payload<Payload>()->_dbg_id; }

PUBLIC inline
L4_obj_ref
Tb_entry_ipc::dst() const
{ return payload<Payload>()->_dst; }

PUBLIC inline
L4_timeout_pair
Tb_entry_ipc::timeout() const
{ return payload<Payload>()->_timeout; }

PUBLIC inline
L4_msg_tag
Tb_entry_ipc::tag() const
{ return payload<Payload>()->_tag; }

PUBLIC inline
Mword
Tb_entry_ipc::label() const
{ return payload<Payload>()->_label; }

PUBLIC inline
Mword
Tb_entry_ipc::dword(unsigned index) const
{ return payload<Payload>()->_dword[index]; }


PUBLIC inline NEEDS ["entry_frame.h"]
void
Tb_entry_ipc_res::set(Context const *ctx, Mword ip, Syscall_frame *ipc_regs,
                      Utcb *utcb,
		      Mword result, Mword pair_event, Unsigned8 have_snd,
		      Unsigned8 is_np)
{
  set_global(Tbuf_ipc_res, ctx, ip);
  Payload *p = payload<Payload>();
  // hint for gcc
  register Mword tmp0 = utcb->values[0];
  register Mword tmp1 = utcb->values[1];
  p->_dword[0]   = tmp0;
  p->_dword[1]   = tmp1;
  p->_tag        = ipc_regs->tag();
  p->_pair_event = pair_event;
  p->_result     = L4_error::from_raw(result);
  p->_from       = ipc_regs->from_spec();
  p->_have_snd   = have_snd;
  p->_is_np      = is_np;
}

PUBLIC inline
int
Tb_entry_ipc_res::have_snd() const
{ return payload<Payload>()->_have_snd; }

PUBLIC inline
int
Tb_entry_ipc_res::is_np() const
{ return payload<Payload>()->_is_np; }

PUBLIC inline
Mword
Tb_entry_ipc_res::from() const
{ return payload<Payload>()->_from; }

PUBLIC inline
L4_error
Tb_entry_ipc_res::result() const
{ return payload<Payload>()->_result; }

PUBLIC inline
L4_msg_tag
Tb_entry_ipc_res::tag() const
{ return payload<Payload>()->_tag; }

PUBLIC inline
Mword
Tb_entry_ipc_res::dword(unsigned index) const
{ return payload<Payload>()->_dword[index]; }

PUBLIC inline
Mword
Tb_entry_ipc_res::pair_event() const
{ return payload<Payload>()->_pair_event; }


PUBLIC inline
void
Tb_entry_ipc_trace::set(Context const *ctx, Mword ip, Unsigned64 snd_tsc,
			L4_obj_ref const &snd_dst, Mword rcv_dst,
			L4_msg_tag result, Unsigned8 snd_desc,
			Unsigned8 rcv_desc)
{
  set_global(Tbuf_ipc_trace, ctx, ip);
  payload<Payload>()->_snd_tsc  = snd_tsc;
  payload<Payload>()->_snd_dst  = snd_dst;
  payload<Payload>()->_rcv_dst  = rcv_dst;
  payload<Payload>()->_result   = result;
  payload<Payload>()->_snd_desc = snd_desc;
  payload<Payload>()->_rcv_desc = rcv_desc;
}

#if 0
PUBLIC inline
void
Tb_entry_ipc_sfl::set(Context *ctx, Mword ip,
		      Global_id from,
		      L4_timeout_pair timeout, Global_id dst,
		      Unsigned8 is_irq, Unsigned8 snd_lst,
		      Unsigned8 dst_ok, Unsigned8 dst_lck,
		      Unsigned8 preempt)
{
  set_global(Tbuf_shortcut_failed, ctx, ip);
  Payload *p = payload<Payload>();
  p->_from      = from;
  p->_timeout   = timeout;
  p->_dst       = dst;
  p->_is_irq    = is_irq;
  p->_snd_lst   = snd_lst;
  p->_dst_ok    = dst_ok;
  p->_dst_lck   = dst_lck;
  p->_preempt   = preempt;
}

PUBLIC inline
L4_timeout_pair
Tb_entry_ipc_sfl::timeout() const
{ return _timeout; }

PUBLIC inline
Global_id
Tb_entry_ipc_sfl::from() const
{ return _from; }

PUBLIC inline
Global_id
Tb_entry_ipc_sfl::dst() const
{ return _dst; }

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::is_irq() const
{ return _is_irq; }

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::snd_lst() const
{ return _snd_lst; }

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::dst_ok() const
{ return _dst_ok; }

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::dst_lck() const
{ return _dst_lck; }

PUBLIC inline
Unsigned8
Tb_entry_ipc_sfl::preempt() const
{ return _preempt; }
#endif

PUBLIC inline
void
Tb_entry_pf::set(Context const *ctx, Address ip, Address pfa,
		 Mword error, Space *spc)
{
  set_global(Tbuf_pf, ctx, ip);
  Payload *p = payload<Payload>();
  p->_pfa   = pfa;
  p->_error = error;
  p->_space = spc;
}

PUBLIC inline
Mword
Tb_entry_pf::error() const
{ return payload<Payload>()->_error; }

PUBLIC inline
Address
Tb_entry_pf::pfa() const
{ return payload<Payload>()->_pfa; }

PUBLIC inline
Space*
Tb_entry_pf::space() const
{ return payload<Payload>()->_space; }


PUBLIC inline
void
Tb_entry_pf_res::set(Context const *ctx, Address ip, Address pfa, 
		     L4_error err, L4_error ret)
{
  set_global(Tbuf_pf_res, ctx, ip);
  Payload *p = payload<Payload>();
  p->_pfa = pfa;
  p->_err = err;
  p->_ret = ret;
}

PUBLIC inline
Address
Tb_entry_pf_res::pfa() const
{ return payload<Payload>()->_pfa; }

PUBLIC inline
L4_error
Tb_entry_pf_res::err() const
{ return payload<Payload>()->_err; }

PUBLIC inline
L4_error
Tb_entry_pf_res::ret() const
{ return payload<Payload>()->_ret; }


PUBLIC inline
void
Tb_entry_bp::set(Context const *ctx, Address ip,
		 int mode, int len, Mword value, Address address)
{
  set_global(Tbuf_breakpoint, ctx, ip);
  payload<Payload>()->_mode    = mode;
  payload<Payload>()->_len     = len;
  payload<Payload>()->_value   = value;
  payload<Payload>()->_address = address;
}



PUBLIC inline
int
Tb_entry_bp::mode() const
{ return payload<Payload>()->_mode; }

PUBLIC inline
int
Tb_entry_bp::len() const
{ return payload<Payload>()->_len; }

PUBLIC inline
Mword
Tb_entry_bp::value() const
{ return payload<Payload>()->_value; }

PUBLIC inline
Address
Tb_entry_bp::addr() const
{ return payload<Payload>()->_address; }



PUBLIC inline
void
Tb_entry_ke::set(Context const *ctx, Address ip)
{ set_global(Tbuf_ke, ctx, ip); }

PUBLIC inline
void
Tb_entry_ke::set_const(Context const *ctx, Address ip, const char * const msg)
{
  set_global(Tbuf_ke, ctx, ip);
  char *_msg = payload<char>();
  _msg[0] = 0; _msg[1] = 1;
  *(char const ** const)(_msg + MSG_POINTER_PAYLOAD_OFFSET) = msg;
}

PUBLIC inline
void
Tb_entry_ke::set_buf(unsigned i, char c)
{
  char *_msg = payload<char>();
  if (i < sizeof(_payload)-1)
    _msg[i] = c >= ' ' ? c : '.';
}

PUBLIC inline
void
Tb_entry_ke::term_buf(unsigned i)
{
  char *_msg = payload<char>();
  _msg[i < sizeof(_payload)-1 ? i : sizeof(_payload)-1] = '\0';
}

PUBLIC inline
const char *
Tb_entry_ke::msg() const
{
  char const *_msg = payload<char>();
  return _msg[0] == 0 && _msg[1] == 1
    ? *(char const ** const)(_msg + MSG_POINTER_PAYLOAD_OFFSET) : _msg;
}


PUBLIC inline
void
Tb_entry_ctx_sw::set(Context const *ctx, Space *from_space, Address ip,
		     Context const *dst, Context const *dst_orig,
		     Mword lock_cnt,
		     Sched_context *from_sched, Mword from_prio,
		     Address kernel_ip)
{
  set_global(Tbuf_context_switch, ctx, ip);
  payload<Payload>()->_kernel_ip = kernel_ip;
  payload<Payload>()->_dst        = dst;
  payload<Payload>()->_dst_orig   = dst_orig;
  payload<Payload>()->_lock_cnt   = lock_cnt;
  payload<Payload>()->_from_space = from_space;
  payload<Payload>()->_from_sched = from_sched;
  payload<Payload>()->_from_prio  = from_prio;
}

PUBLIC inline
Space*
Tb_entry_ctx_sw::from_space() const
{ return payload<Payload>()->_from_space; }

PUBLIC inline
Address
Tb_entry_ctx_sw::kernel_ip() const
{ return payload<Payload>()->_kernel_ip; }

PUBLIC inline
Mword
Tb_entry_ctx_sw::lock_cnt() const
{ return payload<Payload>()->_lock_cnt; }

PUBLIC inline
Context const *
Tb_entry_ctx_sw::dst() const
{ return payload<Payload>()->_dst; }

PUBLIC inline
Context const *
Tb_entry_ctx_sw::dst_orig() const
{ return payload<Payload>()->_dst_orig; }

PUBLIC inline
Mword
Tb_entry_ctx_sw::from_prio() const
{ return payload<Payload>()->_from_prio; }

PUBLIC inline
Sched_context*
Tb_entry_ctx_sw::from_sched() const
{ return payload<Payload>()->_from_sched; }

PUBLIC inline
void
Tb_entry_sched::set (Context const *ctx, Address ip, unsigned short mode,
                     Context const *owner, unsigned short id, unsigned short prio,
                     signed long left, unsigned long quantum)
{
  set_global (Tbuf_sched, ctx, ip);
  payload<Payload>()->_mode    = mode;
  payload<Payload>()->_owner   = owner;
  payload<Payload>()->_id      = id;
  payload<Payload>()->_prio    = prio;
  payload<Payload>()->_left    = left;
  payload<Payload>()->_quantum = quantum;
}

PUBLIC inline
unsigned short
Tb_entry_sched::mode() const
{ return payload<Payload>()->_mode; }

PUBLIC inline
Context const *
Tb_entry_sched::owner() const
{ return payload<Payload>()->_owner; }

PUBLIC inline
unsigned short
Tb_entry_sched::id() const
{ return payload<Payload>()->_id; }

PUBLIC inline
unsigned short
Tb_entry_sched::prio() const
{ return payload<Payload>()->_prio; }

PUBLIC inline
unsigned long
Tb_entry_sched::quantum() const
{ return payload<Payload>()->_quantum; }

PUBLIC inline
signed long
Tb_entry_sched::left() const
{ return payload<Payload>()->_left; }

PUBLIC inline
void
Tb_entry_preemption::set (Context const *ctx, Mword preempter,
                          Address ip)
{
  set_global (Tbuf_preemption, ctx, ip);
  payload<Payload>()->_preempter = preempter;
};

PUBLIC inline
Mword
Tb_entry_preemption::preempter() const
{ return payload<Payload>()->_preempter; }



PUBLIC inline
void
Tb_entry_ke_bin::set(Context const *ctx, Address ip)
{ set_global(Tbuf_ke_bin, ctx, ip); }

PUBLIC inline
void
Tb_entry_ke_bin::set_buf(unsigned i, char c)
{
  char *_bin = payload<char>();
  if (i < sizeof(_payload)-1)
    _bin[i] = c;
}
