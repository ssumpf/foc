INTERFACE [arm]:

EXTENSION class Tb_entry_base
{
public:
  enum
  {
    Tb_entry_size = 64,
  };
  static Mword (*read_cycle_counter)();
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
	char  const*_const_msg;
      };
    };
    Mword         _r0, _r1, _r2;  ///< registers
  };
};

/** logged trap. */
class Tb_entry_trap : public Tb_entry
{
private:
  struct Payload
  {
    Unsigned32    _error;
    Mword         _cpsr, _sp;
  };
};

// --------------------------------------------------------------------
IMPLEMENTATION [arm]:

PROTECTED static Mword Tb_entry_base::dummy_read_cycle_counter() { return 0; }

Mword (*Tb_entry_base::read_cycle_counter)() = dummy_read_cycle_counter;

PUBLIC static
void
Tb_entry_base::set_cycle_read_func(Mword (*f)())
{ read_cycle_counter = f; }

PUBLIC inline
void
Tb_entry::rdtsc()
{ _tsc = read_cycle_counter(); }

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
{ return payload<Payload>()->_r0; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val2() const
{ return payload<Payload>()->_r1; }

PUBLIC inline
Mword
Tb_entry_ke_reg::val3() const
{ return payload<Payload>()->_r2; }

PUBLIC inline
void
Tb_entry_ke_reg::set(Context const *ctx, Mword eip, Mword v1, Mword v2, Mword v3)
{
  set_global(Tbuf_ke_reg, ctx, eip);
  payload<Payload>()->_r0 = v1; payload<Payload>()->_r1 = v2; payload<Payload>()->_r2 = v3;
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
  if (i < sizeof(payload<Payload>()->_msg)-1)
    payload<Payload>()->_msg[i] = c >= ' ' ? c : '.';
}

PUBLIC inline
void
Tb_entry_ke_reg::term_buf(unsigned i)
{ payload<Payload>()->_msg[i < sizeof(payload<Payload>()->_msg)-1 ? i : sizeof(payload<Payload>()->_msg)-1] = '\0'; }

// ------------------
PUBLIC inline
Unsigned16
Tb_entry_trap::cs() const
{ return 0; }

PUBLIC inline
Unsigned8
Tb_entry_trap::trapno() const
{ return 0; }

PUBLIC inline
Unsigned32
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
Tb_entry_trap::set(Context const *ctx, Trap_state *ts)
{
    set_global(Tbuf_trap, ctx, ts->ip());
    payload<Payload>()->_error = ts->error_code;
    payload<Payload>()->_cpsr  = ts->psr;
    payload<Payload>()->_sp    = ts->sp();
}

PUBLIC inline NEEDS ["trap_state.h"]
void
Tb_entry_trap::set(Context const *ctx, Mword pc, Mword )
{
    set_global(Tbuf_trap, ctx, pc);
}
