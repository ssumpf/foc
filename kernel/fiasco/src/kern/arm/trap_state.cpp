
INTERFACE:

#include "l4_types.h"
#include "entry_frame.h"

class Trap_state_regs
{
public:
//  static int (*base_handler)(Trap_state *) asm ("BASE_TRAP_HANDLER");

  Mword pf_address;
  Mword error_code;

  Mword tpidruro;
  Mword r[13];
};

class Trap_state : public Trap_state_regs, public Return_frame
{
public:
  typedef int (*Handler)(Trap_state*, unsigned cpu);
  bool exclude_logging() { return false; }
};


IMPLEMENTATION:

#include <cstdio>
#include "processor.h"

PUBLIC inline NEEDS["processor.h"]
void
Trap_state::sanitize_user_state()
{
  psr &= ~(Proc::Status_mode_mask | Proc::Status_interrupts_mask);
  psr |= Proc::Status_mode_user | Proc::Status_always_mask;
}

PUBLIC inline
void
Trap_state::set_ipc_upcall()
{
  error_code = 0x00600000;
}

PUBLIC inline
void
Trap_state::set_pagefault(Mword pfa, Mword error)
{
  pf_address = pfa;
  error_code = error;
}

PUBLIC inline
unsigned long
Trap_state::ip() const
{ return pc; }

PUBLIC inline
unsigned long
Trap_state::trapno() const
{ return error_code >> 16; }

PUBLIC inline
Mword
Trap_state::error() const
{ return error_code; }

PUBLIC inline
bool
Trap_state::exception_is_undef_insn() const
{ return (error_code & 0x00f00000) == 0x00100000; }

PUBLIC inline
bool
Trap_state::is_debug_exception() const
{ return false; }

PUBLIC
void
Trap_state::dump()
{
  char const *excpts[] =
    { "reset", "undefined insn", "swi", "prefetch abort",
      "data abort", "trigexc", "%&#", "%&#" };

  printf("EXCEPTION: %s pfa=%08lx, error=%08lx\n",
         excpts[(error_code & 0x00700000) >> 20], pf_address, error_code);

  printf("R[0]: %08lx %08lx %08lx %08lx  %08lx %08lx %08lx %08lx\n"
         "R[8]: %08lx %08lx %08lx %08lx  %08lx %08lx %08lx %08lx\n",
	 r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7],
	 r[8], r[9], r[10], r[11], r[12], usp, ulr, pc);
}

