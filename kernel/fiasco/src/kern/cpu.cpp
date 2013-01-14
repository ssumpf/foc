INTERFACE:

#include "cpu_mask.h"
#include "member_offs.h"

class Cpu
{
  MEMBER_OFFSET();

public:
  struct By_phys_id
  {
    Unsigned32 _p;
    By_phys_id(Unsigned32 p) : _p(p) {}
    template<typename CPU>
    bool operator () (CPU const &c) const { return _p == c.phys_id(); }
  };
  // we actually use a mask that has one CPU more that we can physically,
  // have, to avoid lots of special cases for an invalid CPU number
  typedef Cpu_mask_t<Config::Max_num_cpus + 1> Online_cpu_mask;

  enum { Invalid = Config::Max_num_cpus };

  /** Get the logical ID of this CPU */
  unsigned id() const;


  /**
   * Set this CPU to online state.
   * NOTE: This does not activate an inactive CPU, Just set the given state.
   */
  void set_online(bool o);

  /** Convienience for Cpu::cpus.cpu(cpu).online() */
  static bool online(unsigned cpu);

  static Online_cpu_mask const &online_mask();

private:
  /** Is this CPU online ? */
  bool online() const;

  static Online_cpu_mask _online_mask;
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

Cpu::Online_cpu_mask Cpu::_online_mask(Online_cpu_mask::Init::Bss);

IMPLEMENT inline
Cpu::Online_cpu_mask const &
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

IMPLEMENT static inline
bool
Cpu::online(unsigned _cpu)
{ return _cpu == 0; }

