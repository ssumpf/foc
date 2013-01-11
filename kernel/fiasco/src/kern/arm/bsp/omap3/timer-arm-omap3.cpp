INTERFACE [omap3]: // ------------------------------------------------

#include "kmem.h"

class Timer_omap_1mstimer
{
public:
  enum {
    TIDR      = Kmem::Timer1ms_map_base + 0x000, // IP revision code
    TIOCP_CFG = Kmem::Timer1ms_map_base + 0x010, // config
    TISTAT    = Kmem::Timer1ms_map_base + 0x014, // non-interrupt status
    TISR      = Kmem::Timer1ms_map_base + 0x018, // pending interrupts
    TIER      = Kmem::Timer1ms_map_base + 0x01c, // enable/disable of interrupt events
    TWER      = Kmem::Timer1ms_map_base + 0x020, // wake-up features
    TCLR      = Kmem::Timer1ms_map_base + 0x024, // optional features
    TCRR      = Kmem::Timer1ms_map_base + 0x028, // internal counter
    TLDR      = Kmem::Timer1ms_map_base + 0x02c, // timer load value
    TTGR      = Kmem::Timer1ms_map_base + 0x030, // trigger reload by writing
    TWPS      = Kmem::Timer1ms_map_base + 0x034, // write-posted pending
    TMAR      = Kmem::Timer1ms_map_base + 0x038, // compare value
    TCAR1     = Kmem::Timer1ms_map_base + 0x03c, // first capture value of the counter
    TCAR2     = Kmem::Timer1ms_map_base + 0x044, // second capture value of the counter
    TPIR      = Kmem::Timer1ms_map_base + 0x048, // positive inc, gpt1, 2 and 10 only
    TNIR      = Kmem::Timer1ms_map_base + 0x04C, // negative inc, gpt1, 2 and 10 only
  };
};

// --------------------------------------------------------------------------
INTERFACE [arm && omap3_35x]:

EXTENSION class Timer
{
public:
  static unsigned irq() { return 37; }

private:
  enum {
    CM_CLKSEL_WKUP = Kmem::Wkup_cm_map_base + 0x40,
  };
};

INTERFACE [arm && omap3_am33xx]: //----------------------------------------

class Timer_omap_gentimer
{
public:
  enum {
    TIDR          = Kmem::Timergen_map_base + 0x00, // ID
    TIOCP_CFG     = Kmem::Timergen_map_base + 0x10, // config
    EOI           = Kmem::Timergen_map_base + 0x20,
    IRQSTATUS     = Kmem::Timergen_map_base + 0x28,
    IRQENABLE_SET = Kmem::Timergen_map_base + 0x2c,
    IRQWAKEEN     = Kmem::Timergen_map_base + 0x34,
    TCLR          = Kmem::Timergen_map_base + 0x38,
    TCRR          = Kmem::Timergen_map_base + 0x3c,
    TLDR          = Kmem::Timergen_map_base + 0x40,
  };
};

EXTENSION class Timer
{
public:
  enum Timer_type { Timer0, Timer1_1ms };
  static Timer_type type() { return Timer1_1ms; }

  static unsigned irq()
  {
    switch (type())
      {
      case Timer0: default: return 66;
      case Timer1_1ms: return 67;
      }
  }

  enum {
    CM_WKUP_CLKSTCTRL         = Kmem::Cm_wkup_map_base + 0x00,
    CM_WKUP_TIMER0_CLKCTRL    = Kmem::Cm_wkup_map_base + 0x10,
    CM_WKUP_TIMER1_CLKCTRL    = Kmem::Cm_wkup_map_base + 0xc4,
    CLKSEL_TIMER1MS_CLK       = Kmem::Cm_dpll_map_base + 0x28,

    CLKSEL_TIMER1MS_CLK_OSC   = 0,
    CLKSEL_TIMER1MS_CLK_32KHZ = 1,
    CLKSEL_TIMER1MS_CLK_VALUE = CLKSEL_TIMER1MS_CLK_OSC,
  };
};

IMPLEMENTATION [omap3]: // ------------------------------------------------

PRIVATE static
void
Timer_omap_1mstimer::get_timer_values_32khz(unsigned &reload, int &tpir, int &tnir)
{
  tpir   = 232000;
  tnir   = -768000;
  reload = 0xffffffe0;
  assert(Config::Scheduler_granularity == 1000); // need to adapt here
}

IMPLEMENTATION [arm && omap3_35x]: // -------------------------------------

PRIVATE static
void
Timer_omap_1mstimer::get_timer_values(unsigned &reload, int &tpir, int &tnir)
{
  get_timer_values_32khz(reload, tpir, tnir);
}

IMPLEMENTATION [arm && omap3_am33xx]: // ----------------------------------

PRIVATE static
void
Timer_omap_1mstimer::get_timer_values(unsigned &reload, int &tpir, int &tnir)
{
  if (Timer::CLKSEL_TIMER1MS_CLK_VALUE == Timer::CLKSEL_TIMER1MS_CLK_32KHZ)
    get_timer_values_32khz(reload, tpir, tnir);
  else
    {
      tpir   = 100000;
      tnir   = 0;
      reload = ~0 - 24 * Config::Scheduler_granularity + 1; // 24 MHz
    }
}

IMPLEMENTATION [omap3]: // ------------------------------------------------

#include "io.h"
#include <cstdio>

PUBLIC static
void
Timer_omap_1mstimer::init()
{
  // reset
  Io::write<Mword>(1, TIOCP_CFG);
  while (!Io::read<Mword>(TISTAT))
    ;
  // reset done

  // overflow mode
  Io::write<Mword>(0x2, TIER);
  // no wakeup
  Io::write<Mword>(0x0, TWER);

  // program timer frequency
  unsigned val;
  int tpir, tnir;
  get_timer_values(val, tpir, tnir);

  Io::write<Mword>(tpir, TPIR); // gpt1, gpt2 and gpt10 only
  Io::write<Mword>(tnir, TNIR); // gpt1, gpt2 and gpt10 only
  Io::write<Mword>(val,  TCRR);
  Io::write<Mword>(val,  TLDR);

  // auto-reload + enable
  Io::write<Mword>(1 | 2, TCLR);
}

PUBLIC static inline NEEDS["io.h"]
void
Timer_omap_1mstimer::acknowledge()
{
  Io::write<Mword>(2, TISR);
}

IMPLEMENTATION [arm && omap3_am33xx]: // ----------------------------------

PUBLIC static
void
Timer_omap_gentimer::init()
{
  // Mword idr = Io::read<Mword>(TIDR);
  // older timer: idr >> 16 == 0
  // newer timer: idr >> 16 != 0

  // reset
  Io::write<Mword>(1, TIOCP_CFG);
  while (Io::read<Mword>(TIOCP_CFG) & 1)
    ;
  // reset done

  // overflow mode
  Io::write<Mword>(2, IRQENABLE_SET);
  // no wakeup
  Io::write<Mword>(0, IRQWAKEEN);

  // program 1000 Hz timer frequency
  // (FFFFFFFFh - TLDR + 1) * timer-clock-period * clock-divider(ps)
  Mword val = 0xffffffda;
  Io::write<Mword>(val, TLDR);
  Io::write<Mword>(val, TCRR);

  Io::write<Mword>(1 | 2, TCLR);
}

PUBLIC static inline NEEDS["io.h"]
void
Timer_omap_gentimer::acknowledge()
{
  Io::write<Mword>(2, IRQSTATUS);
  Io::write<Mword>(0, EOI);
}

// -----------------------------------------------------------------------
IMPLEMENTATION [arm && omap3_35x]:

IMPLEMENT
void Timer::init(unsigned)
{
  // select 32768 Hz input to GPTimer1 (timer1 only!)
  Io::write<Mword>(~1 & Io::read<Mword>(CM_CLKSEL_WKUP), CM_CLKSEL_WKUP);
  Timer_omap_1mstimer::init();
}

PUBLIC static inline NEEDS[Timer_omap_1mstimer::acknowledge]
void Timer::acknowledge()
{
  Timer_omap_1mstimer::acknowledge();
}

// -----------------------------------------------------------------------
IMPLEMENTATION [arm && omap3_am33xx]:

IMPLEMENT
void
Timer::init(unsigned)
{
  switch (type())
    {
    case Timer1_1ms:
      // enable DMTIMER1_1MS
      Io::write<Mword>(2, CM_WKUP_TIMER1_CLKCTRL);
      Io::read<Mword>(CM_WKUP_TIMER1_CLKCTRL);
      Io::write<Mword>(CLKSEL_TIMER1MS_CLK_VALUE, CLKSEL_TIMER1MS_CLK);
      for (int i = 0; i < 1000000; ++i) // instead, poll proper reg
	asm volatile("" : : : "memory");
      Timer_omap_1mstimer::init();
      break;
    case Timer0:
      Io::write<Mword>(2, CM_WKUP_TIMER0_CLKCTRL);
      Timer_omap_gentimer::init();
      break;
    }
}

PUBLIC static inline NEEDS[Timer_omap_gentimer::acknowledge]
void Timer::acknowledge()
{
  if (type() == Timer1_1ms)
    Timer_omap_1mstimer::acknowledge();
  else
    Timer_omap_gentimer::acknowledge();
}

// -----------------------------------------------------------------------
IMPLEMENTATION [arm && omap3]:

#include "config.h"
#include "kip.h"

static inline
Unsigned64
Timer::timer_to_us(Unsigned32 /*cr*/)
{ return 0; }

static inline
Unsigned64
Timer::us_to_timer(Unsigned64 us)
{ (void)us; return 0; }

IMPLEMENT inline
void
Timer::update_one_shot(Unsigned64 wakeup)
{
  (void)wakeup;
}

IMPLEMENT inline NEEDS["config.h", "kip.h"]
Unsigned64
Timer::system_clock()
{
  if (Config::Scheduler_one_shot)
    return 0;
  else
    return Kip::k()->clock;
}
