// --------------------------------------------------------------------------
IMPLEMENTATION[arm && mptimer]:

PRIVATE static Mword Timer::interval()
{
  Mword v = Io::read<Mword>(Timer_sp804::System_control);
  v |= Timer_sp804::Timclk << Timer_sp804::Timer0_enable;
  Io::write<Mword>(v, Timer_sp804::System_control);

  Mword frequency = 1000000;
  Mword timer_start = ~0UL;
  unsigned factor = 5;
  Mword sp_c = timer_start - frequency / 1000 * (1 << factor);

  Io::write<Mword>(0, Timer_sp804::Ctrl_0);
  Io::write<Mword>(timer_start, Timer_sp804::Value_0);
  Io::write<Mword>(timer_start, Timer_sp804::Load_0);
  Io::write<Mword>(  Timer_sp804::Ctrl_enable
                   | Timer_sp804::Ctrl_periodic,
		   Timer_sp804::Ctrl_0);

  Mword vc = start_as_counter();
  while (sp_c < Io::read<Mword>(Timer_sp804::Value_0))
    ;
  Mword interval = (vc - stop_counter()) >> factor;
  Io::write<Mword>(0, Timer_sp804::Ctrl_0);
  return interval;
}
