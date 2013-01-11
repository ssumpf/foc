/*
 * Fiasco
 * Floating point unit code
 */

INTERFACE:

#include "initcalls.h"
#include "per_cpu_data.h"
#include "types.h"

class Context;
class Fpu_state;
class Trap_state;


class Fpu
{
public:

  static Context *owner(unsigned cpu);
  static void set_owner(unsigned cpu, Context *owner);
  static bool is_owner(unsigned cpu, Context *owner);

  // all the following methods are arch dependent
  static void init(unsigned cpu) FIASCO_INIT_CPU;
  static unsigned state_size();
  static unsigned state_align();
  static void init_state(Fpu_state *);
  static void restore_state(Fpu_state *);
  static void save_state(Fpu_state *);
  static void disable();
  static void enable();

private:
  Context *_owner;

  static Per_cpu<Fpu> _fpu;
};

IMPLEMENTATION:

#include "fpu_state.h"

DEFINE_PER_CPU Per_cpu<Fpu> Fpu::_fpu;

IMPLEMENT inline
Context * Fpu::owner(unsigned cpu)
{
  return _fpu.cpu(cpu)._owner;
}

IMPLEMENT inline
void Fpu::set_owner(unsigned cpu, Context *owner)
{
  _fpu.cpu(cpu)._owner = owner;
}

IMPLEMENT inline
bool Fpu::is_owner(unsigned cpu, Context *owner)
{
  return _fpu.cpu(cpu)._owner == owner;
}

PUBLIC static inline
Fpu &
Fpu::fpu(unsigned cpu)
{
  return _fpu.cpu(cpu);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [!fpu]:

IMPLEMENT inline
void Fpu::init_state(Fpu_state *)
{}

IMPLEMENT inline
unsigned Fpu::state_size()
{ return 0; }

IMPLEMENT inline
unsigned Fpu::state_align()
{ return 1; }

IMPLEMENT
void Fpu::init(unsigned)
{}

IMPLEMENT inline
void Fpu::save_state(Fpu_state *)
{}

IMPLEMENT inline
void Fpu::restore_state(Fpu_state *)
{}

IMPLEMENT inline
void Fpu::disable()
{}

IMPLEMENT inline
void Fpu::enable()
{}
