INTERFACE:

#include "cpu_mask.h"
#include "member_offs.h"

class Cpu
{
  MEMBER_OFFSET();

public:
  /** Get the locical ID of this CPU */
  unsigned id() const;


  /**
   * Set this CPU to online state.
   * NOTE: This does not activate an inactive CPU, Just set the given state.
   */
  void set_online(bool o);

  /** Get the physical ID of the CPU, for inter processor communication */
  unsigned phys_id() const;

  /** Convienience for Cpu::cpus.cpu(cpu).online() */
  static bool online(unsigned cpu);

  /**
   * Get logical CPU id from physical ID
   * NOTE: This call is SLOW, use only for debugging/bootup
   */
  static unsigned p2l(unsigned phys_id);

  static Cpu_mask const &online_mask();


private:
  /** Is this CPU online ? */
  bool online() const;

  static Cpu_mask _online_mask;
};


//--------------------------------------------------------------------------
INTERFACE[mp]:

EXTENSION class Cpu
{

private:
  void set_id(unsigned id) { _id = id; }
  unsigned _id;
};

//--------------------------------------------------------------------------
INTERFACE[!mp]:

EXTENSION class Cpu
{
private:
  void set_id(unsigned) {}
};


// --------------------------------------------------------------------------
IMPLEMENTATION:

Cpu_mask Cpu::_online_mask(Cpu_mask::Init::Bss);

IMPLEMENT inline
Cpu_mask const &
Cpu::online_mask()
{ return _online_mask; }

// --------------------------------------------------------------------------
IMPLEMENTATION [mp]:

#include "kdb_ke.h"

IMPLEMENT inline
unsigned
Cpu::id() const
{ return _id; }

IMPLEMENT inline
bool
Cpu::online() const
{ return _online_mask.get(_id); }

IMPLEMENT inline
void
Cpu::set_online(bool o)
{
  if (o)
    _online_mask.set(_id);
  else
    _online_mask.clear(_id);
}

IMPLEMENT
unsigned
Cpu::p2l(unsigned phys_id)
{
  for (unsigned i = 0; i < Config::Max_num_cpus; ++i)
    if (Per_cpu_data::valid(i) && Cpu::cpus.cpu(i).phys_id() == phys_id)
      return i;

  return ~0U;
}

IMPLEMENT static inline NEEDS["kdb_ke.h"]
bool
Cpu::online(unsigned _cpu)
{ return _online_mask.get(_cpu); }


// --------------------------------------------------------------------------
IMPLEMENTATION [!mp]:

IMPLEMENT inline
unsigned
Cpu::id() const
{ return 0; }

IMPLEMENT inline
bool
Cpu::online() const
{ return true; }

IMPLEMENT inline
void
Cpu::set_online(bool)
{}

IMPLEMENT
unsigned
Cpu::p2l(unsigned)
{ return 0; }

IMPLEMENT static inline
bool
Cpu::online(unsigned _cpu)
{ return _cpu == 0; }

