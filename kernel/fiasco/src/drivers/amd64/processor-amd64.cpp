INTERFACE[amd64]:

EXTENSION class Proc
{
public:
  enum Efer_bits
  {
    Efer_sce_flag  = 0x00000001,      // Syscall Enable Flag
    Efer_lme_flag  = 0x00000100,      // Long Mode Enable Flag
    Efer_nxe_flag  = 0x00000800,      // Not-executable	     
    Efer_svme_flag = 0x00001000,      // Enable SVM
  };
};

IMPLEMENTATION[amd64]:

#include "types.h"
#include "std_macros.h"

IMPLEMENT static inline
Mword Proc::stack_pointer()
{
  Mword sp;
  asm volatile ("mov %%rsp, %0 \n" : "=r"(sp) );
  return sp;
}

IMPLEMENT static inline
void Proc::stack_pointer(Mword sp)
{
  asm volatile ("mov %0, %%rsp \n" : : "r"(sp) );
}

IMPLEMENT static inline
Mword ALWAYS_INLINE Proc::program_counter()
{
  Mword pc;
  asm volatile ("call 1f ; 1: pop %0" : "=r"(pc));
  return pc;
}

IMPLEMENT static inline
void Proc::pause()
{
  asm volatile (" .byte 0xf3, 0x90 #pause \n" ); 
}

/*
 * The following simple ASM statements need the clobbering to work around
 * a bug in (at least) gcc-3.2.x up to x == 1. The bug was fixed on
 * Jan 9th 2003 (see gcc-bugs #9242 and #8832), so a released gcc-3.2.2
 * won't have it. It's safe to take the clobber statements out after
 * some time (e.g. when gcc-3.3 is used as a standard compiler).
 */

IMPLEMENT static inline
void Proc::halt()
{
  asm volatile ("hlt" : : : "memory");
}

IMPLEMENT static inline
void Proc::cli()
{
  asm volatile ("cli" : : : "memory");
}

IMPLEMENT static inline
void Proc::sti()
{
  asm volatile ("sti" : : : "memory");
}

IMPLEMENT static inline
Proc::Status Proc::cli_save()
{
  Status ret;
  asm volatile ("pushfq	\n\t"
		"popq %0	\n\t"
		"cli		\n\t"
		: "=g"(ret) : /* no input */ : "memory");
  return ret;
}

IMPLEMENT static inline
void Proc::sti_restore(Status st)
{
  if (st & 0x0200)
    asm volatile ("sti" : : : "memory");
}

IMPLEMENT static inline
Proc::Status Proc::interrupts()
{
  Status ret;
  asm volatile ("pushfq         \n"
		"popq %0        \n"
		: "=g"(ret) : /* no input */ : "memory");
  return ret & 0x0200;
}

IMPLEMENT static inline
void Proc::irq_chance()
{
  asm volatile ("nop");
  pause();
}


PUBLIC static inline
Unsigned64
Proc::rdmsr(Unsigned32 msr)
{
  Unsigned32 h,l;

  asm volatile ("rdmsr" : "=a" (l), "=d" (h) : "c" (msr));
  return (((Mword)h) << 32) | l;
}

PUBLIC static inline
void
Proc::wrmsr(Unsigned32 msr, Unsigned64 value)
{
  asm volatile ("wrmsr" : : 
      "a" ((Unsigned32)value),
      "d" ((Unsigned32)(value >> 32)),
      "c" (msr)); 
}

PUBLIC static inline
Mword
Proc::efer()
{ return rdmsr(0xc0000080); }

PUBLIC static inline
void
Proc::efer(Mword value)
{ wrmsr(0xc0000080, value); }


