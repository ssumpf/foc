INTERFACE [amd64]:

EXTENSION class Tb_entry_base
{
public:
  enum
  {
    Tb_entry_size = 128,
  };
};

/** logged kernel event plus register content. */
class Tb_entry_ke_reg : public Tb_entry
{
private:
  struct Payload
  {
    union {
      char _msg[19];	///< debug message
      struct {
	char _pad[3];
	const char *_m;
      } _const_msg __attribute__((packed));
    };
    Mword		_rax, _rcx, _rdx; ///< registers
  } __attribute__((packed));
};

/** logged trap. */
class Tb_entry_trap : public Tb_entry
{
private:
  struct Payload
  {
    char	_trapno;
    Unsigned16	_error;
    Mword	_rbp, _cr2, _rax, _rflags, _rsp;
    Unsigned16	_cs,  _ds;
  } __attribute__((packed));
};

IMPLEMENTATION [amd64]:

#include "cpu.h"

PUBLIC inline NEEDS ["cpu.h"]
void
Tb_entry::rdtsc()
{ _tsc = Cpu::rdtsc(); }



PUBLIC inline
void
Tb_entry_ke_reg::set(Context const *ctx, Mword rip, Mword v1, Mword v2, Mword v3)
{
  set_global(Tbuf_ke_reg, ctx, rip);
  payload<Payload>()->_rax = v1; payload<Payload>()->_rcx = v2; payload<Payload>()->_rdx = v3;
}

PUBLIC inline NEEDS [<cstring>]
void
Tb_entry_ke_reg::set(Context const *ctx, Mword rip, Trap_state *ts)
{ set(ctx, rip, ts->_ax, ts->_cx, ts->_dx); }

PUBLIC inline
void
Tb_entry_ke_reg::set_const(Context const *ctx, Mword rip,
                           const char * const msg,
                           Mword rax, Mword rcx, Mword rdx)
{
  set(ctx, rip, rax, rcx, rdx);
  payload<Payload>()->_msg[0] = 0;
  payload<Payload>()->_msg[1] = 1;
  payload<Payload>()->_const_msg._m = msg;
}

PUBLIC inline
void
Tb_entry_ke_reg::set_buf(unsigned i, char c)
{
  if (i < sizeof(payload<Payload>()->_msg) - 1)
    payload<Payload>()->_msg[i] = c >= ' ' ? c : '.';
}

PUBLIC inline
void
Tb_entry_ke_reg::term_buf(unsigned i)
{ payload<Payload>()->_msg[i < sizeof(payload<Payload>()->_msg)-1 ? i : sizeof(payload<Payload>()->_msg)-1] = '\0'; }

PUBLIC inline
const char *
Tb_entry_ke_reg::msg() const
{
  return payload<Payload>()->_msg[0] == 0 &&
         payload<Payload>()->_msg[1] == 1 ? payload<Payload>()->_const_msg._m
                                          : payload<Payload>()->_msg;
}

PUBLIC inline
Mword
Tb_entry_ke_reg::val1() const
{ return payload<Payload>()->_rax; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val2() const
{ return payload<Payload>()->_rcx; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val3() const
{ return payload<Payload>()->_rdx; }


PUBLIC inline NEEDS ["trap_state.h"]
void
Tb_entry_trap::set(Context const *ctx, Mword rip, Trap_state *ts)
{
  set_global(Tbuf_trap, ctx, rip);
  payload<Payload>()->_trapno = ts->_trapno;
  payload<Payload>()->_error  = ts->_err;
  payload<Payload>()->_cr2    = ts->_cr2;
  payload<Payload>()->_rax    = ts->_ax;
  payload<Payload>()->_cs     = (Unsigned16)ts->cs();
  payload<Payload>()->_rsp    = ts->sp();
  payload<Payload>()->_rflags = ts->flags();
}

PUBLIC inline
void
Tb_entry_trap::set(Context const *ctx, Mword eip, Mword trapno)
{
  set_global(Tbuf_trap, ctx, eip);
  payload<Payload>()->_trapno = trapno | 0x80;
}

PUBLIC inline
char
Tb_entry_trap::trapno() const
{ return payload<Payload>()->_trapno; }

PUBLIC inline
Unsigned16
Tb_entry_trap::error() const
{ return payload<Payload>()->_error; }

PUBLIC inline
Mword
Tb_entry_trap::eax() const
{ return payload<Payload>()->_rax; }

PUBLIC inline
Mword
Tb_entry_trap::cr2() const
{ return payload<Payload>()->_cr2; }

PUBLIC inline
Mword
Tb_entry_trap::ebp() const
{ return payload<Payload>()->_rbp; }

PUBLIC inline
Unsigned16
Tb_entry_trap::cs() const
{ return payload<Payload>()->_cs; }

PUBLIC inline
Unsigned16
Tb_entry_trap::ds() const
{ return payload<Payload>()->_ds; }

PUBLIC inline
Mword
Tb_entry_trap::sp() const
{ return payload<Payload>()->_rsp; }

PUBLIC inline
Mword
Tb_entry_trap::flags() const
{ return payload<Payload>()->_rflags; }


