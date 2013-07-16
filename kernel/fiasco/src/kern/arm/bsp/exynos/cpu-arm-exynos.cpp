IMPLEMENTATION [arm && bsp_cpu]:

PRIVATE
void
Cpu::bsp_init(bool is_boot_cpu)
{
  // enable FOZ -- do this in conjunction with L2 cache
  if (is_boot_cpu)
    set_actrl(1 << 3);
}
