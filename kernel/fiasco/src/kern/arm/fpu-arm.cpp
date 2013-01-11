INTERFACE [arm && fpu]:

EXTENSION class Fpu
{
public:
  struct Exception_state_user
  {
    Mword fpexc;
    Mword fpinst;
    Mword fpinst2;
  };

  Mword fpsid() const { return _fpsid; }

  enum
  {
    FPEXC_EN  = 1 << 30,
    FPEXC_EX  = 1 << 31,
  };

  struct Fpu_regs
  {
    Mword fpexc, fpscr;
    Mword state[32 * 4]; // 4*32 bytes for each FP-reg
  };

  static Mword fpsid_rev(Mword v)          { return v & 0xf; }
  static Mword fpsid_variant(Mword v)      { return (v >> 4) & 0xf; }
  static Mword fpsid_part_number(Mword v)  { return (v >> 8) & 0xff; }
  static Mword fpsid_arch_version(Mword v) { return (v >> 16) & 0xf; }
  static Mword fpsid_precision(Mword v)    { return (v >> 20) & 1; }
  static Mword fpsid_format(Mword v)       { return (v >> 21) & 3; }
  static Mword fpsid_hw_sw(Mword v)        { return (v >> 23) & 1; }
  static Mword fpsid_implementer(Mword v)  { return v >> 24; }

private:
  Mword _fpsid;
};

// ------------------------------------------------------------------------
INTERFACE [arm && !fpu]:

EXTENSION class Fpu
{
public:
  struct Exception_state_user
  {
  };
};

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && !fpu]:

#include "trap_state.h"

PUBLIC static inline NEEDS["trap_state.h"]
void
Fpu::save_user_exception_state(Trap_state *, Exception_state_user *)
{}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && fpu]:

#include <cassert>
#include <cstdio>
#include <cstring>

#include "fpu_state.h"
#include "mem.h"
#include "processor.h"
#include "static_assert.h"
#include "trap_state.h"

PUBLIC static inline
Mword
Fpu::fpsid_read()
{
  Mword v;
  asm volatile("mrc p10, 7, %0, cr0, cr0" : "=r" (v));
  return v;
}

PUBLIC static inline
Mword
Fpu::mvfr0()
{
  Mword v;
  asm volatile("mrc p10, 7, %0, cr7, cr0" : "=r" (v));
  return v;
}

PUBLIC static inline
Mword
Fpu::mvfr1()
{
  Mword v;
  asm volatile("mrc p10, 7, %0, cr6, cr0" : "=r" (v));
  return v;
}


PRIVATE static inline
void
Fpu::fpexc(Mword v)
{
  asm volatile("mcr p10, 7, %0, cr8, cr0" : : "r" (v));
}

PUBLIC static inline
Mword
Fpu::fpexc()
{
  Mword v;
  asm volatile("mrc p10, 7, %0, cr8, cr0" : "=r" (v));
  return v;
}

PUBLIC static inline
Mword
Fpu::fpinst()
{
  Mword i;
  asm volatile("mcr p10, 7, %0, cr9,  cr0" : "=r" (i));
  return i;
}

PUBLIC static inline
Mword
Fpu::fpinst2()
{
  Mword i;
  asm volatile("mcr p10, 7, %0, cr10,  cr0" : "=r" (i));
  return i;
}

PUBLIC static inline
bool
Fpu::exc_pending()
{
  return fpexc() & FPEXC_EX;
}

IMPLEMENT
void
Fpu::enable()
{
  fpexc((fpexc() | FPEXC_EN) & ~FPEXC_EX);
}

IMPLEMENT
void
Fpu::disable()
{
  fpexc(fpexc() & ~FPEXC_EN);
}

PUBLIC static inline
int
Fpu::is_emu_insn(Mword opcode)
{
  return (opcode & 0x0ff00f90) == 0x0ef00a10;
}

PUBLIC static inline
bool
Fpu::emulate_insns(Mword opcode, Trap_state *ts, unsigned cpu)
{
  unsigned reg = (opcode >> 16) & 0xf;
  unsigned rt  = (opcode >> 12) & 0xf;
  Mword fpsid = Fpu::fpu(cpu).fpsid();
  switch (reg)
    {
    case 0: // FPSID
      ts->r[rt] = fpsid;
      break;
    case 6: // MVFR1
      if (Fpu::fpsid_arch_version(fpsid) < 2)
        return false;
      ts->r[rt] = Fpu::mvfr1();
      break;
    case 7: // MVFR0
      if (Fpu::fpsid_arch_version(fpsid) < 2)
        return false;
      ts->r[rt] = Fpu::mvfr0();
      break;
    default:
      break;
    }

  if (ts->psr & Proc::Status_thumb)
    ts->pc += 2;

  return true;
}



IMPLEMENT
void
Fpu::init(unsigned cpu)
{
  asm volatile(" mcr  p15, 0, %0, c1, c0, 2   \n" : : "r"(0x00f00000));
  Mem::dsb();

  Mword s = fpsid_read();

  _fpu.cpu(cpu)._fpsid = s;

  printf("FPU%d: Arch: %s(%lx), Part: %s(%lx), r: %lx, v: %lx, i: %lx, t: %s, p: %s\n",
         cpu, fpsid_arch_version(s) == 1
           ? "VFPv2"
           : (fpsid_arch_version(s) == 3 ? "VFPv3" : "Unkn"),
         fpsid_arch_version(s),
         fpsid_part_number(s) == 0x20
           ? "VFP11"
           : (fpsid_part_number(s) == 0x30 ?  "VFPv3" : "Unkn"),
         fpsid_part_number(s),
         fpsid_rev(s), fpsid_variant(s), fpsid_implementer(s),
         fpsid_hw_sw(s) ? "soft" : "hard",
         fpsid_precision(s) ? "sngl" : "dbl/sngl");

  disable();

  set_owner(cpu, 0);
}

IMPLEMENT inline NEEDS ["fpu_state.h", "mem.h", "static_assert.h", <cstring>]
void
Fpu::init_state (Fpu_state *s)
{
  Fpu_regs *fpu_regs = reinterpret_cast<Fpu_regs *>(s->state_buffer());
  static_assert(!(sizeof (*fpu_regs) % sizeof(Mword)),
                "Non-mword size of Fpu_regs");
  Mem::memset_mwords(fpu_regs, 0, sizeof (*fpu_regs) / sizeof(Mword));
}

IMPLEMENT
void
Fpu::save_state(Fpu_state *s)
{
  assert(s->state_buffer());
  Fpu_regs *fpu_regs = reinterpret_cast<Fpu_regs *>(s->state_buffer());

  asm volatile ("stc p11, cr0, [%0], #32*4     \n"
                : : "r" (fpu_regs->state));
  asm volatile ("mrc p10, 7, %0, cr8,  cr0, 0  \n"
                "mrc p10, 7, %1, cr1,  cr0, 0  \n"
                : "=r" (fpu_regs->fpexc),
                  "=r" (fpu_regs->fpscr));
}

IMPLEMENT
void
Fpu::restore_state (Fpu_state *s)
{
  assert (s->state_buffer());
  Fpu_regs *fpu_regs = reinterpret_cast<Fpu_regs *>(s->state_buffer());

  asm volatile ("ldc p11, cr0, [%0], #32*4     \n"
                : : "r" (fpu_regs->state));
  asm volatile ("mcr p10, 7, %0, cr8,  cr0, 0  \n"
                "mcr p10, 7, %1, cr1,  cr0, 0  \n"
                :
                : "r" (fpu_regs->fpexc | FPEXC_EN),
                  "r" (fpu_regs->fpscr));

#if 0
  asm volatile("mcr p10, 7, %2, cr9,  cr0, 0  \n"
               "mcr p10, 7, %3, cr10, cr0, 0  \n"
               :
               : "r" (fpu_regs->fpinst),
                 "r" (fpu_regs->fpinst2));
#endif
}

IMPLEMENT inline
unsigned
Fpu::state_size()
{ return sizeof (Fpu_regs); }

IMPLEMENT inline
unsigned
Fpu::state_align()
{ return 4; }

PUBLIC static
bool
Fpu::is_enabled()
{
  return fpexc() & FPEXC_EN;
}

PUBLIC static inline NEEDS["trap_state.h"]
void
Fpu::save_user_exception_state(Trap_state *ts, Exception_state_user *esu)
{
  if ((ts->error_code & 0x01f00000) == 0x01100000)
    {
      esu->fpexc = Fpu::fpexc();
      if (ts->error_code == 0x03100000)
	{
	  esu->fpinst  = Fpu::fpinst();
	  esu->fpinst2 = Fpu::fpinst2();
	}
    }
}
