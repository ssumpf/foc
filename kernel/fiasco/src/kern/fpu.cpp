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
  // all the following methods are arch dependent
  static void init(unsigned cpu) FIASCO_INIT_CPU;

  static unsigned state_size();
  static unsigned state_align();
  static void init_state(Fpu_state *);
  static void restore_state(Fpu_state *);
  static void save_state(Fpu_state *);
  static void disable();
  static void enable();

  static Per_cpu<Fpu> fpu;

  Context *owner() const { return _owner; }
  void set_owner(Context *owner) { _owner = owner; }
  bool is_owner(Context *owner) const { return _owner == owner; }

private:
  Context *_owner;
};

IMPLEMENTATION:

#include "fpu_state.h"

DEFINE_PER_CPU Per_cpu<Fpu> Fpu::fpu;


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
