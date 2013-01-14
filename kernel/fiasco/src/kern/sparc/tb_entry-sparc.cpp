INTERFACE [sparc]:

EXTENSION class Tb_entry
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
      char          _msg[16];       ///< debug message
      struct {
        char        _dsc[2];
        char const* _const_msg;
      };
    };
    Mword         _r3, _r4, _r5;    ///< registers
  };
};

/** logged trap. */
class Tb_entry_trap : public Tb_entry
{
private:
  struct Payload
  {
    Unsigned8     _trapno;
    Unsigned16    _error;
    Mword         _cpsr, _sp;
  };
};

// --------------------------------------------------------------------
IMPLEMENTATION [sparc]:

PUBLIC inline
void
Tb_entry::rdtsc()
{}

PUBLIC inline
const char *
Tb_entry_ke_reg::msg() const
{
  return payload<Payload>()->_dsc[0] == 0 && payload<Payload>()->_dsc[1] == 1
    ? payload<Payload>()->_const_msg : payload<Payload>()->_msg;
}

PUBLIC inline
Mword
Tb_entry_ke_reg::val1() const
{ return payload<Payload>()->_r3; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val2() const
{ return payload<Payload>()->_r4; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val3() const
{ return payload<Payload>()->_r5; }

PUBLIC inline
void
Tb_entry_ke_reg::set(Context const *ctx, Mword eip, Mword v1, Mword v2, Mword v3)
{
  set_global(Tbuf_ke_reg, ctx, eip);
  payload<Payload>()->_r3 = v1;
  payload<Payload>()->_r4 = v2;
  payload<Payload>()->_r5 = v3;
}

PUBLIC inline
void
Tb_entry_ke_reg::set_const(Context const *ctx, Mword eip, const char * const msg,
                           Mword v1, Mword v2, Mword v3)
{
  set(ctx, eip, v1, v2, v3);
  payload<Payload>()->_dsc[0] = 0; payload<Payload>()->_dsc[1] = 1;
  payload<Payload>()->_const_msg = msg;
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
{payload<Payload>()->_msg[i < sizeof(payload<Payload>()->_msg)-1 ? i
  : sizeof(payload<Payload>()->_msg)-1] = '\0';}

// ------------------
PUBLIC inline
Unsigned16
Tb_entry_trap::cs() const
{ return 0; }

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
Tb_entry_trap::sp() const
{ return payload<Payload>()->_sp; }

PUBLIC inline
Mword
Tb_entry_trap::cr2() const
{ return 0; }

PUBLIC inline
Mword
Tb_entry_trap::eax() const
{ return 0; }

PUBLIC inline NEEDS ["trap_state.h"]
void
Tb_entry_trap::set(Context const *ctx, Mword pc, Trap_state *)
{
  set_global(Tbuf_trap, ctx, pc);
  // More...
}

PUBLIC inline NEEDS ["trap_state.h"]
void
Tb_entry_trap::set(Context const *ctx, Mword pc, Mword )
{
  set_global(Tbuf_trap, ctx, pc);
}

