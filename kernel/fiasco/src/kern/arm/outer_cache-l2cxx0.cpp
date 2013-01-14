INTERFACE [arm && outer_cache_l2cxx0]:

#include "mem_layout.h"
#include "spin_lock.h"

EXTENSION class Outer_cache
{
private:
  enum
  {
    Cache_line_shift = 5,

    CACHE_ID                       = Mem_layout::L2cxx0_map_base + 0x000,
    CACHE_TYPE                     = Mem_layout::L2cxx0_map_base + 0x004,
    CONTROL                        = Mem_layout::L2cxx0_map_base + 0x100,
    AUX_CONTROL                    = Mem_layout::L2cxx0_map_base + 0x104,
    TAG_RAM_CONTROL                = Mem_layout::L2cxx0_map_base + 0x108,
    DATA_RAM_CONTROL               = Mem_layout::L2cxx0_map_base + 0x10c,
    EVENT_COUNTER_CONTROL          = Mem_layout::L2cxx0_map_base + 0x200,
    EVENT_COUTNER1_CONFIG          = Mem_layout::L2cxx0_map_base + 0x204,
    EVENT_COUNTER0_CONFIG          = Mem_layout::L2cxx0_map_base + 0x208,
    EVENT_COUNTER1_VALUE           = Mem_layout::L2cxx0_map_base + 0x20c,
    EVENT_COUNTER0_VALUE           = Mem_layout::L2cxx0_map_base + 0x210,
    INTERRUPT_MASK                 = Mem_layout::L2cxx0_map_base + 0x214,
    MASKED_INTERRUPT_STATUS        = Mem_layout::L2cxx0_map_base + 0x218,
    RAW_INTERRUPT_STATUS           = Mem_layout::L2cxx0_map_base + 0x21c,
    INTERRUPT_CLEAR                = Mem_layout::L2cxx0_map_base + 0x220,
    CACHE_SYNC                     = Mem_layout::L2cxx0_map_base + 0x730,
    INVALIDATE_LINE_BY_PA          = Mem_layout::L2cxx0_map_base + 0x770,
    INVALIDATE_BY_WAY              = Mem_layout::L2cxx0_map_base + 0x77c,
    CLEAN_LINE_BY_PA               = Mem_layout::L2cxx0_map_base + 0x7b0,
    CLEAN_LINE_BY_INDEXWAY         = Mem_layout::L2cxx0_map_base + 0x7bb,
    CLEAN_BY_WAY                   = Mem_layout::L2cxx0_map_base + 0x7bc,
    CLEAN_AND_INV_LINE_BY_PA       = Mem_layout::L2cxx0_map_base + 0x7f0,
    CLEAN_AND_INV_LINE_BY_INDEXWAY = Mem_layout::L2cxx0_map_base + 0x7f8,
    CLEAN_AND_INV_BY_WAY           = Mem_layout::L2cxx0_map_base + 0x7fc,
    LOCKDOWN_BY_WAY_D_SIDE         = Mem_layout::L2cxx0_map_base + 0x900,
    LOCKDOWN_BY_WAY_I_SIDE         = Mem_layout::L2cxx0_map_base + 0x904,
    TEST_OPERATION                 = Mem_layout::L2cxx0_map_base + 0xf00,
    LINE_TAG                       = Mem_layout::L2cxx0_map_base + 0xf30,
    DEBUG_CONTROL_REGISTER         = Mem_layout::L2cxx0_map_base + 0xf40,
  };

  static Spin_lock<> _lock;

  static Mword platform_init(Mword aux);

  static bool need_sync;
  static unsigned waymask;

public:
  enum
  {
    Cache_line_size = 1 << Cache_line_shift,
    Cache_line_mask = Cache_line_size - 1,
  };
};

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && outer_cache_l2cxx0]:

#include "io.h"
#include "lock_guard.h"
#include "static_init.h"

Spin_lock<> Outer_cache::_lock;
bool Outer_cache::need_sync;
unsigned Outer_cache::waymask;

IMPLEMENT inline NEEDS ["io.h"]
void
Outer_cache::sync()
{
  while (Io::read<Mword>(CACHE_SYNC))
    Proc::preemption_point();
}

PRIVATE static inline NEEDS ["io.h", "lock_guard.h"]
void
Outer_cache::write(Address reg, Mword val, bool before = false)
{
  auto guard = lock_guard(_lock);
  if (before)
    while (Io::read<Mword>(reg) & 1)
      ;
  Io::write<Mword>(val, reg);
  if (!before)
    while (Io::read<Mword>(reg) & 1)
      ;
}

IMPLEMENT inline NEEDS[Outer_cache::write]
void
Outer_cache::clean()
{
  write(CLEAN_BY_WAY, waymask);
  sync();
}

IMPLEMENT inline NEEDS[Outer_cache::write]
void
Outer_cache::clean(Mword phys_addr, bool do_sync = true)
{
  write(CLEAN_LINE_BY_PA, phys_addr & (~0UL << Cache_line_shift));
  if (need_sync && do_sync)
    sync();
}

IMPLEMENT inline NEEDS[Outer_cache::write]
void
Outer_cache::flush()
{
  write(CLEAN_AND_INV_BY_WAY, waymask);
  sync();
}

IMPLEMENT inline NEEDS[Outer_cache::write]
void
Outer_cache::flush(Mword phys_addr, bool do_sync = true)
{
  write(CLEAN_AND_INV_LINE_BY_PA, phys_addr & (~0UL << Cache_line_shift));
  if (need_sync && do_sync)
    sync();
}

IMPLEMENT inline NEEDS[Outer_cache::write]
void
Outer_cache::invalidate()
{
  write(INVALIDATE_BY_WAY, waymask);
  sync();
}

IMPLEMENT inline NEEDS[Outer_cache::write]
void
Outer_cache::invalidate(Address phys_addr, bool do_sync = true)
{
  write(INVALIDATE_LINE_BY_PA, phys_addr & (~0UL << Cache_line_shift));
  if (need_sync && do_sync)
    sync();
}

PUBLIC static
void
Outer_cache::initialize(bool v)
{
  Mword cache_id   = Io::read<Mword>(CACHE_ID);
  Mword aux        = Io::read<Mword>(AUX_CONTROL);
  unsigned ways    = 8;

  need_sync = true;

  aux = platform_init(aux);

  switch ((cache_id >> 6) & 0xf)
    {
    case 1:
      ways = (aux >> 13) & 0xf;
      break;
    case 3:
      need_sync = false;
      ways = aux & (1 << 16) ? 16 : 8;
      break;
    default:
      break;
    }

  waymask = (1 << ways) - 1;

  Io::write<Mword>(0,    INTERRUPT_MASK);
  Io::write<Mword>(~0UL, INTERRUPT_CLEAR);

  if (!(Io::read<Mword>(CONTROL) & 1))
    {
      Io::write(aux, AUX_CONTROL);
      invalidate();
      Io::write<Mword>(1, CONTROL);
    }

  if (v)
    show_info(ways, cache_id, aux);
}

PUBLIC static
void
Outer_cache::init()
{
  initialize(true);
}

STATIC_INITIALIZE_P(Outer_cache, STARTUP_INIT_PRIO);

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && outer_cache_l2cxx0 && !debug]:

PRIVATE static
void
Outer_cache::show_info(unsigned, Mword, Mword)
{}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && outer_cache_l2cxx0 && debug]:

#include "io.h"
#include <cstdio>

PRIVATE static
void
Outer_cache::show_info(unsigned ways, Mword cache_id, Mword aux)
{
  printf("L2: ID=%08lx Type=%08lx Aux=%08lx WMask=%x S=%d\n",
         cache_id, Io::read<Mword>(CACHE_TYPE), aux, waymask, need_sync);

  const char *type;
  switch ((cache_id >> 6) & 0xf)
    {
    case 1:
      type = "210";
      break;
    case 2:
      type = "220";
      break;
    case 3:
      type = "310";
      if (cache_id & 0x3f == 5)
        printf("L2: r3p0\n");
      break;
    default:
      type = "Unknown";
      break;
    }

  unsigned waysize = 16 << (((aux >> 17) & 7) - 1);
  printf("L2: Type L2C-%s Size = %dkB\n", type, ways * waysize);
}
