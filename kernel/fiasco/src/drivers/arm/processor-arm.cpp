INTERFACE[arm]:

EXTENSION class Proc
{
public:
  enum
  {
    Status_mode_user           = 0x10,
    Status_mode_supervisor     = 0x13,
    Status_mode_mask           = 0x1f,

    Status_FIQ_disabled        = 0x40,
    Status_IRQ_disabled        = 0x80,
    Status_interrupts_disabled = Status_FIQ_disabled | Status_IRQ_disabled,
    Status_interrupts_mask     = 0xc0,
    Status_thumb               = 0x20,
  };

  static unsigned cpu_id();
};

INTERFACE[arm && !tz]:

EXTENSION class Proc
{
public:
  enum
    {
      Cli_mask = Status_IRQ_disabled,
      Sti_mask = Status_IRQ_disabled,
    };
};

INTERFACE[arm && tz]:

EXTENSION class Proc
{
public:
  enum
    {
      Cli_mask = Status_IRQ_disabled | Status_FIQ_disabled,
      Sti_mask = Status_IRQ_disabled | Status_FIQ_disabled,
    };
};

IMPLEMENTATION[arm]:

#include "types.h"
#include "std_macros.h"

IMPLEMENT static inline
Mword Proc::stack_pointer()
{
  Mword sp;
  asm volatile ( "mov %0, sp \n" : "=r"(sp) );
  return sp;
}

IMPLEMENT static inline
void Proc::stack_pointer(Mword sp)
{
  asm volatile ( "mov sp, %0 \n" : : "r"(sp) );
}

IMPLEMENT static inline
Mword Proc::program_counter()
{
  register Mword pc asm ("pc");
  return pc;
}

IMPLEMENT static inline
void Proc::cli()
{
  asm volatile ( "    mrs    r6, cpsr    \n"
		 "    orr    r6,r6,%0    \n"
		 "    msr    cpsr_c, r6  \n"
		 : : "i" (Cli_mask) : "r6", "memory"
		 );
}

IMPLEMENT static inline
void Proc::sti()
{
  asm volatile ( "    mrs    r6, cpsr    \n"
                 "    bic    r6,r6,%0   \n"
		 "    msr    cpsr_c, r6  \n"
		 : : "i" (Sti_mask) : "r6", "memory"
		 );
}

IMPLEMENT static inline
Proc::Status Proc::cli_save()
{
  Status ret;
  asm volatile ( "    mrs    r6, cpsr    \n"
		 "    mov    %0, r6      \n"
		 "    orr    r6,r6,%1    \n"
		 "    msr    cpsr_c, r6  \n"
		 : "=r"(ret) : "i" (Cli_mask) : "r6" 
		 );
  return ret;
}

IMPLEMENT static inline
Proc::Status Proc::interrupts()
{
  Status ret;
  asm volatile ("   mrs  %0, cpsr  \n"
		: "=r"(ret)
		);
  return !(ret & Sti_mask);
}

IMPLEMENT static inline
void Proc::sti_restore(Status st)
{
  asm volatile ( "    tst    %0, %1      \n"
		 "    bne    1f          \n"
		 "    mrs    r6, cpsr    \n"
		 "    bic    r6,r6,%1    \n"
		 "    msr    cpsr_c, r6  \n"
		 "1:                     \n"
		 : : "r"(st), "i" (Sti_mask) : "r6"
		 );
}

IMPLEMENT static inline
void Proc::irq_chance()
{
  asm volatile ("nop; nop;" : : : "memory");
}


//----------------------------------------------------------------
IMPLEMENTATION[arm && !mp]:

IMPLEMENT static inline
unsigned Proc::cpu_id()
{ return 0; }

//----------------------------------------------------------------
IMPLEMENTATION[arm && mp]:

IMPLEMENT static inline
unsigned Proc::cpu_id()
{
  unsigned int cpunum;
  __asm__("mrc p15, 0, %0, c0, c0, 5": "=r" (cpunum));
  return cpunum & 0xf;
}

//----------------------------------------------------------------
IMPLEMENTATION[arm && (pxa || sa1100 || s3c2410)]:

IMPLEMENT static inline
void Proc::halt()
{}

IMPLEMENT static inline
void Proc::pause()
{}

//----------------------------------------------------------------
IMPLEMENTATION[arm && 926]:

IMPLEMENT static inline
void Proc::pause()
{
}

IMPLEMENT static inline
void Proc::halt()
{
  Status f = cli_save();
  asm volatile("mov     r0, #0                                              \n\t"
	       "mrc     p15, 0, r1, c1, c0, 0       @ Read control register \n\t"
	       "mcr     p15, 0, r0, c7, c10, 4      @ Drain write buffer    \n\t"
	       "bic     r2, r1, #1 << 12                                    \n\t"
	       "mcr     p15, 0, r2, c1, c0, 0       @ Disable I cache       \n\t"
	       "mcr     p15, 0, r0, c7, c0, 4       @ Wait for interrupt    \n\t"
	       "mcr     15, 0, r1, c1, c0, 0        @ Restore ICache enable \n\t"
	       :::"memory",
	       "r0", "r1", "r2", "r3", "r4", "r5",
	       "r6", "r7", "r8", "r9", "r10", "r11",
	       "r12", "r13", "r14", "r15"
      );
  sti_restore(f);
}

//----------------------------------------------------------------
IMPLEMENTATION[arm && arm1136]:

IMPLEMENT static inline
void Proc::pause()
{}

IMPLEMENT static inline
void Proc::halt()
{
  Status f = cli_save();
  asm volatile("mcr     p15, 0, r0, c7, c10, 4  @ DWB/DSB \n\t"
               "mcr     p15, 0, r0, c7, c0, 4   @ WFI \n\t");
  sti_restore(f);
}


//----------------------------------------------------------------
IMPLEMENTATION[arm && (arm1176 || mpcore)]:

IMPLEMENT static inline
void Proc::pause()
{}

IMPLEMENT static inline
void Proc::halt()
{
  Status f = cli_save();
  asm volatile("mcr     p15, 0, r0, c7, c10, 4  @ DWB/DSB \n\t"
               "wfi \n\t");
  sti_restore(f);
}

//----------------------------------------------------------------
IMPLEMENTATION[arm && (armca8 || armca9)]:

IMPLEMENT static inline
void Proc::pause()
{}

IMPLEMENT static inline
void Proc::halt()
{
  Status f = cli_save();
  asm volatile("dsb \n\t"
               "wfi \n\t");
  sti_restore(f);
}
