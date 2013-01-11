INTERFACE [ia32,ux]:

EXTENSION class Tb_entry_base
{
public:
  enum
  {
    Tb_entry_size = 64,
  };
};

/** logged kernel event plus register content. */
class Tb_entry_ke_reg : public Tb_entry
{
private:
  struct Payload
  {
    union {
      char  _msg[18];        ///< debug message
      struct {
	char _pad[3];
	const char *_m;
      } _const_msg __attribute__((packed));
    };
    Mword  _eax, _ecx, _edx; ///< registers
  } __attribute__((packed));
};

/** logged trap. */
class Tb_entry_trap : public Tb_entry
{
private:
  struct Payload
  {
    Unsigned8	_trapno;
    Unsigned16	_error;
    Mword	_ebp, _cr2, _eax, _eflags, _esp;
    Unsigned16	_cs,  _ds;
  } __attribute__((packed));
};

IMPLEMENTATION [ia32,ux]:

#include "cpu.h"

PUBLIC inline NEEDS ["cpu.h"]
void
Tb_entry::rdtsc()
{ _tsc = Cpu::rdtsc(); }



PUBLIC inline
void
Tb_entry_ke_reg::set(Context const *ctx, Mword eip, Mword v1, Mword v2, Mword v3)
{
  set_global(Tbuf_ke_reg, ctx, eip);
  payload<Payload>()->_eax = v1; payload<Payload>()->_ecx = v2; payload<Payload>()->_edx = v3;
}

PUBLIC inline NEEDS [<cstring>]
void
Tb_entry_ke_reg::set(Context const *ctx, Mword eip, Trap_state *ts)
{ set(ctx, eip, ts->value(), ts->value2(), ts->value3()); }

PUBLIC inline
void
Tb_entry_ke_reg::set_const(Context const *ctx, Mword eip,
                           const char * const msg,
                           Mword eax, Mword ecx, Mword edx)
{
  set(ctx, eip, eax, ecx, edx);
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
{ return payload<Payload>()->_eax; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val2() const
{ return payload<Payload>()->_ecx; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val3() const
{ return payload<Payload>()->_edx; }


PUBLIC inline NEEDS ["trap_state.h"]
void
Tb_entry_trap::set(Context const *ctx, Mword eip, Trap_state *ts)
{
  set_global(Tbuf_trap, ctx, eip);
  payload<Payload>()->_trapno = ts->_trapno;
  payload<Payload>()->_error  = ts->_err;
  payload<Payload>()->_cr2    = ts->_cr2;
  payload<Payload>()->_eax    = ts->_ax;
  payload<Payload>()->_cs     = (Unsigned16)ts->cs();
  payload<Payload>()->_ds     = (Unsigned16)ts->_ds;
  payload<Payload>()->_esp    = ts->sp();
  payload<Payload>()->_eflags = ts->flags();
}

PUBLIC inline
void
Tb_entry_trap::set(Context const *ctx, Mword eip, Mword trapno)
{
  set_global(Tbuf_trap, ctx, eip);
  payload<Payload>()->_trapno = trapno;
  payload<Payload>()->_cs     = 0;
}

PUBLIC inline
Unsigned8
Tb_entry_trap::trapno() const
{ return payload<Payload>()->_trapno; }

PUBLIC inline
Unsigned16
Tb_entry_trap::error() const
{ return payload<Payload>()->_error; }

PUBLIC inline
Mword
Tb_entry_trap::eax() const
{ return payload<Payload>()->_eax; }

PUBLIC inline
Mword
Tb_entry_trap::cr2() const
{ return payload<Payload>()->_cr2; }

PUBLIC inline
Mword
Tb_entry_trap::ebp() const
{ return payload<Payload>()->_ebp; }

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
{ return payload<Payload>()->_esp; }

PUBLIC inline
Mword
Tb_entry_trap::flags() const
{ return payload<Payload>()->_eflags; }
