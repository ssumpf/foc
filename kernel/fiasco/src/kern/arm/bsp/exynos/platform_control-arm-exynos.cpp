INTERFACE [arm && exynos]:

#include "types.h"
#include "io.h"
#include "mem_layout.h"
#include "mmio_register_block.h"

EXTENSION class Platform_control
{
public:

  class Pmu : public Mmio_register_block
  {
  public:
    explicit Pmu(Address virt) : Mmio_register_block(virt) {}
    enum Reg
    {
      Config      = 0,
      Status      = 4,
      Option      = 8,
      Core_offset = 0x80,

      ARM_COMMON_OPTION      = 0x2408,
   };

    static Mword core_pwr_reg(Cpu_phys_id cpu, unsigned func)
    { return 0x2000 + Core_offset * cxx::int_value<Cpu_phys_id>(cpu) + func; }

    Mword read(unsigned reg) const
    { return Mmio_register_block::read<Mword>(reg); }

    void write(Mword val, unsigned reg) const
    { Mmio_register_block::write(val, reg); }

    Mword core_read(Cpu_phys_id cpu, unsigned reg) const
    { return Mmio_register_block::read<Mword>(core_pwr_reg(cpu, reg)); }

    void core_write(Mword val, Cpu_phys_id cpu, unsigned reg) const
    { Mmio_register_block::write(val, core_pwr_reg(cpu, reg)); }
  };

  enum Power_down_mode
  {
    Aftr, Lpa, Dstop, Sleep,

    Pwr_down_mode = Aftr,
  };

  enum Pmu_regs
  {
    Pmu_core_local_power_enable = 3,
  };

  static Static_object<Pmu> pmu;
};


//--------------------------------------------------------------------------
IMPLEMENTATION [arm && exynos && !mp]:

PRIVATE static
int
Platform_control::power_up_core(Cpu_phys_id)
{
  return -L4_err::ENodev;
}

//--------------------------------------------------------------------------
IMPLEMENTATION [arm && exynos]:

Static_object<Platform_control::Pmu> Platform_control::pmu;

IMPLEMENT
void
Platform_control::init(Cpu_number cpu)
{
  if (cpu == Cpu_number::boot_cpu())
    {
      assert (!pmu->get_mmio_base());
      pmu.construct(Kmem::mmio_remap(Mem_layout::Pmu_phys_base));

      for (Cpu_phys_id i = Cpu_phys_id(0);
           i < Cpu_phys_id(2);
           ++i)
        pmu->core_write((pmu->core_read(i, Pmu::Option) & ~(1 << 0)) | (1 << 1), i, Pmu::Option);

      pmu->write(2, Pmu::ARM_COMMON_OPTION);
    }
}

//--------------------------------------------------------------------------
IMPLEMENTATION [arm && exynos && mp]:

#include "ipi.h"
#include "outer_cache.h"
#include "platform.h"
#include "poll_timeout_kclock.h"
#include "smc.h"

PRIVATE static
int
Platform_control::power_up_core(Cpu_phys_id cpu)
{
  // CPU already powered up?
  if ((pmu->core_read(cpu, Pmu::Status) & Pmu_core_local_power_enable) != 0)
    return 0;

  pmu->core_write(Pmu_core_local_power_enable, cpu, Pmu::Config);

  Lock_guard<Cpu_lock, Lock_guard_inverse_policy> cpu_lock_guard(&cpu_lock);

  Poll_timeout_kclock pt(10000);
  while (pt.test((pmu->core_read(cpu, Pmu::Status)
                  & Pmu_core_local_power_enable)
                 != Pmu_core_local_power_enable))
      ;

  return pt.timed_out() ? -L4_err::ENodev : 0;
}

PRIVATE static
void
Platform_control::set_one_vector(Mword addr_p, Mword vector)
{
  Mword addr_v = Kmem::mmio_remap(addr_p);
  Io::write<Mword>(vector, addr_v);
  Mem_unit::flush_dcache((void *)addr_v, (void *)(addr_v + sizeof(void *)));
  Outer_cache::flush(addr_p);
}

PUBLIC static
void
Platform_control::setup_cpu_start_vector(Cpu_phys_id cpu, Mword phys_reset_vector)
{
  unsigned long b = Mem_layout::Sysram_phys_base;

  if (Platform::running_ns())
    b += Platform::is_4210() ? 0x1f01c : 0x2f01c;
  else if (Platform::is_4210() && Platform::subrev() == 0x11)
    b = Mem_layout::Pmu_phys_base + 0x814;
  else if (Platform::is_4210() && Platform::subrev() == 0)
    b += 0x5000;

  if (Platform::is_4412())
    b += cxx::int_value<Cpu_phys_id>(cpu) * 4;

  set_one_vector(b, phys_reset_vector);
}


PUBLIC static
void
Platform_control::boot_ap_cpus(Address phys_reset_vector)
{
  assert(current_cpu() == Cpu_number::boot_cpu());

  if (Platform::is_4412())
    {
      for (Cpu_phys_id i = Cpu_phys_id(1);
           i < Cpu_phys_id(4) && i < Cpu_phys_id(Config::Max_num_cpus);
           ++i)
        {
          setup_cpu_start_vector(i, phys_reset_vector);
          power_up_core(i);
          if (Platform::running_ns())
            Exynos_smc::cpuboot(phys_reset_vector,
                                cxx::int_value<Cpu_phys_id>(i));
          Ipi::send(Ipi::Global_request, Cpu_number::boot_cpu(), i);
        }

      return;
    }

  Mem_unit::flush_dcache();

  Cpu_phys_id const second = Cpu_phys_id(1);
  setup_cpu_start_vector(second, phys_reset_vector);
  power_up_core(second);

  if (Platform::is_5250() && Platform::running_ns())
    Exynos_smc::cpuboot(phys_reset_vector, 1);

  Ipi::send(Ipi::Global_request, Cpu_number::boot_cpu(), second);
}
