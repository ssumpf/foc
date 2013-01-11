IMPLEMENTATION [arm && tegra2 && outer_cache_l2cxx0]:

IMPLEMENT
Mword
Outer_cache::platform_init(Mword aux_control)
{
  Io::write<Mword>(0x331, TAG_RAM_CONTROL);
  Io::write<Mword>(0x441, DATA_RAM_CONTROL);

  aux_control &= 0x8200c3fe;
  aux_control |=   (1 <<  0)  // Full Line of Zero Enable
                 | (4 << 17)  // 128kb waysize
                 | (1 << 28)  // data prefetch
                 | (1 << 29)  // insn prefetch
                 | (1 << 30)  // early BRESP enable
		 ;
  return aux_control;
}
