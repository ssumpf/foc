INTERFACE:

class Ipi
{
public:
  static void init(unsigned cpu);
};

INTERFACE[!mp]:

EXTENSION class Ipi
{
public:
  enum Message { Request, Global_request, Debug };
};

INTERFACE[mp]:

#include "per_cpu_data.h"
#include "spin_lock.h"

EXTENSION class Ipi
{
private:
  static Per_cpu<Ipi> _ipi;
};

// ------------------------------------------------------------------------
IMPLEMENTATION[!mp]:

IMPLEMENT inline
void
Ipi::init(unsigned cpu)
{ (void)cpu; }

PUBLIC static inline
void
Ipi::send(Message, unsigned from_cpu, unsigned to_cpu)
{ (void)from_cpu; (void)to_cpu; }

PUBLIC static inline
void
Ipi::eoi(Message, unsigned on_cpu)
{ (void)on_cpu; }

PUBLIC static inline
void
Ipi::bcast(Message, unsigned from_cpu)
{ (void)from_cpu; }


// ------------------------------------------------------------------------
IMPLEMENTATION[mp]:

DEFINE_PER_CPU Per_cpu<Ipi> Ipi::_ipi;

// ------------------------------------------------------------------------
IMPLEMENTATION[!(mp && debug)]:

PUBLIC static inline
void
Ipi::stat_sent(unsigned from_cpu)
{ (void)from_cpu; }

PUBLIC static inline
void
Ipi::stat_received(unsigned on_cpu)
{ (void)on_cpu; }

// ------------------------------------------------------------------------
IMPLEMENTATION[mp && debug]:

#include "globals.h"

EXTENSION class Ipi
{
private:
  friend class Jdb_ipi_module;
  Mword _stat_sent;
  Mword _stat_received;
};

PUBLIC static inline
void
Ipi::stat_sent(unsigned from_cpu)
{ atomic_mp_add(&_ipi.cpu(from_cpu)._stat_sent, 1); }

PUBLIC static inline
void
Ipi::stat_received(unsigned on_cpu)
{ _ipi.cpu(on_cpu)._stat_received++; }
