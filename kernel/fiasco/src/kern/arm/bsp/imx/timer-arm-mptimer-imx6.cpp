// --------------------------------------------------------------------------
IMPLEMENTATION[arm && imx6 && mptimer]:

#include "config.h"
#include "io.h"
#include "mem_layout.h"

PRIVATE static Mword Timer::interval()
{
  enum
  {
    GPT_CR  = Mem_layout::Gpt_map_base + 0x00,
    GPT_PR  = Mem_layout::Gpt_map_base + 0x04,
    GPT_SR  = Mem_layout::Gpt_map_base + 0x08,
    GPT_IR  = Mem_layout::Gpt_map_base + 0x0c,
    GPT_CNT = Mem_layout::Gpt_map_base + 0x24,

    GPT_CR_EN                 = 1 << 0,
    GPT_CR_CLKSRC_MASK        = 7 << 6,
    GPT_CR_CLKSRC_CRYSTAL_OSC = 7 << 6,
    GPT_CR_CLKSRC_32KHZ       = 4 << 6,
    GPT_CR_FRR                = 1 << 9,
    GPT_CR_RESET              = 1 << 15,

    Timer_freq = 32768,
    Ticks = 50,
    Gpt_ticks = (Timer_freq * Ticks) / Config::Scheduler_granularity,
  };

  Io::write<Mword>(0, GPT_CR);
  Io::write<Mword>(GPT_CR_RESET, GPT_CR);
  while (Io::read<Mword>(GPT_CR) & GPT_CR_RESET)
    ;

  Io::write<Mword>(GPT_CR_CLKSRC_32KHZ | GPT_CR_FRR, GPT_CR);
  Io::write<Mword>(0, GPT_PR);

  Io::set<Mword>(GPT_CR_EN, GPT_CR);
  Mword vc = start_as_counter();
  while (Io::read<Mword>(GPT_CNT) < Gpt_ticks)
    ;
  Mword interval = (vc - stop_counter()) / Ticks;
  Io::write<Mword>(0, GPT_CR);
  return interval;
}
