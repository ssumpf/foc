INTERFACE[arm]:

EXTENSION class Proc
{
private:
  enum : unsigned
  {
    Status_FIQ_disabled        = 0x40,
    Status_IRQ_disabled        = 0x80,
  };

public:
  enum : unsigned
  {
    Status_mode_user           = 0x10,
    Status_mode_supervisor     = 0x13,
    Status_mode_mask           = 0x1f,

    Status_interrupts_disabled = Status_FIQ_disabled | Status_IRQ_disabled,
    Status_thumb               = 0x20,
  };

  static Cpu_phys_id cpu_id();
};

INTERFACE[arm && !arm_em_tz && !arm_em_ns]:

EXTENSION class Proc
{
public:
  enum : unsigned
    {
      Cli_mask                = Status_interrupts_disabled,
      Sti_mask                = Status_interrupts_disabled,
      Status_preempt_disabled = Status_IRQ_disabled,
      Status_interrupts_mask  = Status_interrupts_disabled,
      Status_always_mask      = 0,
    };
};

INTERFACE[arm && arm_em_tz]:

EXTENSION class Proc
{
public:
  enum : unsigned
    {
      Cli_mask                = Status_FIQ_disabled,
      Sti_mask                = Status_FIQ_disabled,
      Status_preempt_disabled = Status_FIQ_disabled,
      Status_interrupts_mask  = Status_FIQ_disabled,
      Status_always_mask      = Status_IRQ_disabled,
    };
};

INTERFACE[arm && arm_em_ns]:

EXTENSION class Proc
{
public:
  enum : unsigned
    {
      Cli_mask                = Status_IRQ_disabled,
      Sti_mask                = Status_IRQ_disabled,
      Status_preempt_disabled = Status_IRQ_disabled,
      Status_interrupts_mask  = Status_IRQ_disabled,
      Status_always_mask      = Status_FIQ_disabled,
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
  Mword v;
  asm volatile("mrs %0, cpsr    \n"
               "orr %0, %0, %1  \n"
               "msr cpsr_c, %0  \n"
               : "=r" (v)
               : "I" (Cli_mask)
               : "memory");
}

IMPLEMENT static inline
void Proc::sti()
{
  Mword v;
  asm volatile("mrs %0, cpsr    \n"
               "bic %0, %0, %1  \n"
               "msr cpsr_c, %0  \n"
               : "=r" (v)
               : "I" (Sti_mask)
               : "memory");
}

IMPLEMENT static inline
Proc::Status Proc::cli_save()
{
  Status ret;
  Mword v;
  asm volatile("mrs    %0, cpsr    \n"
               "orr    %1, %0, %2  \n"
               "msr    cpsr_c, %1  \n"
               : "=r" (ret), "=r" (v)
               : "I" (Cli_mask)
               : "memory");
  return ret;
}

IMPLEMENT static inline
Proc::Status Proc::interrupts()
{
  Status ret;
  asm volatile("mrs %0, cpsr" : "=r" (ret));
  return !(ret & Sti_mask);
}

IMPLEMENT static inline
void Proc::sti_restore(Status st)
{
  if (!(st & Sti_mask))
    sti();
}

IMPLEMENT static inline
void Proc::irq_chance()
{
  asm volatile ("nop; nop;" : : : "memory");
}


//----------------------------------------------------------------
IMPLEMENTATION[arm && !mp]:

IMPLEMENT static inline
Cpu_phys_id Proc::cpu_id()
{ return Cpu_phys_id(0); }

//----------------------------------------------------------------
IMPLEMENTATION[arm && mp]:

IMPLEMENT static inline
Cpu_phys_id Proc::cpu_id()
{
  unsigned mpidr;
  __asm__("mrc p15, 0, %0, c0, c0, 5": "=r" (mpidr));
  return Cpu_phys_id(mpidr & 0x7); // mind gic softirq
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
{
  asm("yield");
}

IMPLEMENT static inline
void Proc::halt()
{
  Status f = cli_save();
  asm volatile("dsb \n\t"
               "wfi \n\t");
  sti_restore(f);
}
