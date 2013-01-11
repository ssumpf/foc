IMPLEMENTATION [mp]:

#include "cpu.h"
#include "pic.h"
#include "gic.h"
#include "processor.h"

EXTENSION class Ipi
{
private:
  Unsigned32 _phys_id;

public:
  enum Message
  {
    Ipi_start = 1,
    Global_request = Ipi_start, Request, Debug,
    Ipi_end
  };
};


PUBLIC inline
Ipi::Ipi() : _phys_id(~0)
{}

IMPLEMENT inline NEEDS["processor.h"]
void
Ipi::init(unsigned cpu)
{
  _ipi.cpu(cpu)._phys_id = Proc::cpu_id();
}

PUBLIC static
void Ipi::ipi_call_debug_arch()
{
}

PUBLIC static inline
void Ipi::eoi(Message, unsigned on_cpu)
{
  // with the ARM-GIC we have to do the EOI right after the ACK
  stat_received(on_cpu);
}

// ---------------------------------------------------------------------------
IMPLEMENTATION [mp && !irregular_gic]:

PUBLIC static inline NEEDS["pic.h"]
void Ipi::send(Message m, unsigned from_cpu, unsigned to_cpu)
{
  Pic::gic->softint_cpu(1 << _ipi.cpu(to_cpu)._phys_id, m);
  stat_sent(from_cpu);
}

PUBLIC static inline
void
Ipi::bcast(Message m, unsigned from_cpu)
{
  (void)from_cpu;
  Pic::gic->softint_bcast(m);
}

