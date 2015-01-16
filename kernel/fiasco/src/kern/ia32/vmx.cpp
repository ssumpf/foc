INTERFACE [vmx]:

#include "per_cpu_data.h"
#include "pm.h"

#include <cassert>
#include <cstdio>
#include <cstring>

class Vmx_info
{
public:

  template<typename T>
  class Bit_defs
  {
  protected:
    T _or;
    T _and;

  public:
    Bit_defs() {}
    Bit_defs(T _or, T _and) : _or(_or), _and(_and) {}

    T must_be_one() const { return _or; }
    T may_be_one() const { return _and; }

  private:
    void enforce_bits(T m, bool value = true)
    {
      if (value)
	_or |= m;
      else
	_and &= ~m;
    }

    bool allowed_bits(T m, bool value = true) const
    {
      if (value)
	return _and & m;
      else
	return !(_or & m);
    }

  public:
    void relax(unsigned char bit)
    {
      _or &= ~(T(1) << T(bit));
      _and |= T(1) << T(bit);
    }

    void enforce(unsigned char bit, bool value = true)
    { enforce_bits((T)1 << (T)bit, value); }

    bool allowed(unsigned char bit, bool value = true) const
    { return allowed_bits((T)1 << (T)bit, value); }

    T apply(T v) const { return (v | _or) & _and; }

    void print(char const *name) const
    {
      if (sizeof(T) <= 4)
	printf("%20s = %8x %8x\n", name, (unsigned)_and, (unsigned)_or);
      else if (sizeof(T) <= 8)
	printf("%20s = %16llx %16llx\n", name, (unsigned long long)_and, (unsigned long long)_or);
    }
  };

  class Bit_defs_32 : public Bit_defs<Unsigned32>
  {
  public:
    Bit_defs_32() {}
    Bit_defs_32(Unsigned64 v) : Bit_defs<Unsigned32>(v, v >> 32) {}
  };

  typedef Bit_defs<Unsigned64> Bit_defs_64;

  template<typename T>
  class Flags
  {
  public:
    Flags() {}
    explicit Flags(T v) : _f(v) {}

    T test(unsigned char bit) const { return _f & ((T)1 << (T)bit); }
  private:
    T _f;
  };

  Unsigned64 basic;

  Bit_defs_32 pinbased_ctls;
  Bit_defs_32 procbased_ctls;

  Bit_defs_32 exit_ctls;
  Bit_defs_32 entry_ctls;
  Unsigned64 misc;

  Bit_defs<Mword> cr0_defs;
  Bit_defs<Mword> cr4_defs;
  Bit_defs_32 procbased_ctls2;

  Unsigned64 ept_vpid_cap;
  Unsigned64 max_index;
  Unsigned32 pinbased_ctls_default1;
  Unsigned32 procbased_ctls_default1;
  Unsigned32 exit_ctls_default1;
  Unsigned32 entry_ctls_default1;
};


struct Vmx_user_info
{
  struct Fo_table
  {
    unsigned char offsets[32];
    static unsigned char const master_offsets[32];

    enum
    {
      Foi_size = 4
    };

    static void *field(void *b, unsigned vm_field)
    {
      return (char*)b + master_offsets[vm_field >> 10] * 64
             + ((vm_field & 0x3ff) << master_offsets[Foi_size + (vm_field >> 13)]);
    }

    void init()
    { memcpy(offsets, master_offsets, sizeof(offsets)); }

    static void check_offsets(unsigned max_idx)
    {
      for (unsigned t1 = 0; t1 < 3; ++t1)
        for (unsigned w1 = 0; w1 < 4; ++w1)
          for (unsigned t2 = 0; t2 < 3; ++t2)
            for (unsigned w2 = 0; w2 < 4; ++w2)
              if (t1 != t2 || w1 != w2)
                {
                  unsigned s1 = ((t1 << 10) | (w1 << 13));
                  unsigned s2 = ((t2 << 10) | (w2 << 13));
                  unsigned e1 = s1 | max_idx;
                  unsigned e2 = s2 | max_idx;
                  assert (field(0, s1) > field(0, e2)
                          || field(0, s2) > field(0, e1));
                  (void) s1; (void) s2; (void) e1; (void) e2;
                }
    }
  };

  Unsigned64 basic;
  Vmx_info::Bit_defs_32 pinbased;
  Vmx_info::Bit_defs_32 procbased;
  Vmx_info::Bit_defs_32 exit;
  Vmx_info::Bit_defs_32 entry;
  Unsigned64 misc;
  Unsigned64 cr0_or;
  Unsigned64 cr0_and;
  Unsigned64 cr4_or;
  Unsigned64 cr4_and;
  Unsigned64 vmcs_field_info;
  Vmx_info::Bit_defs_32 procbased2;
  Unsigned64 ept_vpid_cap;
  Unsigned32 pinbased_dfl1;
  Unsigned32 procbased_dfl1;
  Unsigned32 exit_dfl1;
  Unsigned32 entry_dfl1;
};

INTERFACE:

class Vmx : public Pm_object
{
public:
  enum Vmcs_fields
  {
    F_vpid               = 0x0,

    F_host_es_selector   = 0x0c00,
    F_host_cs_selector   = 0x0c02,
    F_host_ss_selector   = 0x0c04,
    F_host_ds_selector   = 0x0c06,
    F_host_fs_selector   = 0x0c08,
    F_host_gs_selector   = 0x0c0a,
    F_host_tr_selector   = 0x0c0c,

    F_tsc_offset         = 0x2010,
    F_apic_access_addr   = 0x2014,

    F_guest_pat          = 0x2804,
    F_guest_efer         = 0x2806,

    F_host_ia32_pat              = 0x2c00,
    F_host_ia32_efer             = 0x2c02,
    F_host_ia32_perf_global_ctrl = 0x2c04,


    F_pin_based_ctls     = 0x4000,
    F_proc_based_ctls    = 0x4002,

    F_cr3_target_cnt     = 0x400a,
    F_exit_ctls          = 0x400c,
    F_exit_msr_store_cnt = 0x400e,
    F_exit_msr_load_cnt  = 0x4010,
    F_entry_ctls         = 0x4012,
    F_entry_msr_load_cnt = 0x4014,
    F_entry_int_info     = 0x4016,

    F_entry_exc_error_code = 0x4018,
    F_entry_insn_len     = 0x401a,
    F_proc_based_ctls_2  = 0x401e,

    F_vm_instruction_error = 0x4400,
    F_exit_reason        = 0x4402,

    F_preempt_timer      = 0x482e,

    F_host_sysenter_cs   = 0x4c00,

    F_guest_cr3          = 0x6802,
    F_sw_guest_cr2       = 0x683e,


    F_host_cr0           = 0x6c00,
    F_host_cr3           = 0x6c02,
    F_host_cr4           = 0x6c04,
    F_host_fs_base       = 0x6c06,
    F_host_gs_base       = 0x6c08,
    F_host_tr_base       = 0x6c0a,
    F_host_gdtr_base     = 0x6c0c,
    F_host_idtr_base     = 0x6c0e,
    F_host_sysenter_esp  = 0x6c10,
    F_host_sysenter_eip  = 0x6c12,
    F_host_rip           = 0x6c16,

  };

  enum Pin_based_ctls
  {
    PIB_ext_int_exit = 0,
    PIB_nmi_exit     = 3,
  };

  enum Primary_proc_based_ctls
  {
    PRB1_tpr_shadow               = 21,
    PRB1_unconditional_io_exit    = 24,
    PRB1_use_io_bitmaps           = 25,
    PRB1_use_msr_bitmaps          = 28,
    PRB1_enable_proc_based_ctls_2 = 31,
  };

  enum Secondary_proc_based_ctls
  {
    PRB2_virtualize_apic = 0,
    PRB2_enable_ept      = 1,
    PRB2_enable_vpid     = 5,
    PRB2_unrestricted    = 7,
  };
};

INTERFACE [vmx]:

#include "virt.h"
#include "cpu_lock.h"

class Vmx_info;

EXTENSION class Vmx
{
public:
  static Per_cpu<Vmx> cpus;
  Vmx_info info;
private:
  void *_vmxon;
  bool _vmx_enabled;
  bool _has_vpid;
  Unsigned64 _vmxon_base_pa;
  void *_kernel_vmcs;
  Unsigned64 _kernel_vmcs_pa;
};

class Vmx_info_msr
{
private:
  Unsigned64 _data;
};

//-----------------------------------------------------------------------------
INTERFACE [vmx && ia32]:

EXTENSION class Vmx
{
public:
  enum { Gpregs_words = 11 };
};

//-----------------------------------------------------------------------------
INTERFACE [vmx && amd64]:

EXTENSION class Vmx
{
public:
  enum { Gpregs_words = 19 };
};


// -----------------------------------------------------------------------
IMPLEMENTATION[vmx]:

#include "cpu.h"
#include "kmem.h"
#include "l4_types.h"
#include <cstring>
#include "idt.h"
#include "warn.h"

DEFINE_PER_CPU_LATE Per_cpu<Vmx> Vmx::cpus(Per_cpu_data::Cpu_num);

PUBLIC
void
Vmx_info::init()
{
  bool ept = false;
  basic = Cpu::rdmsr(0x480);
  pinbased_ctls = Cpu::rdmsr(0x481);
  pinbased_ctls_default1 = pinbased_ctls.must_be_one();
  procbased_ctls = Cpu::rdmsr(0x482);
  procbased_ctls_default1 = procbased_ctls.must_be_one();
  exit_ctls = Cpu::rdmsr(0x483);
  exit_ctls_default1 = exit_ctls.must_be_one();
  entry_ctls = Cpu::rdmsr(0x484);
  entry_ctls_default1 = entry_ctls.must_be_one();
  misc = Cpu::rdmsr(0x485);

  cr0_defs = Bit_defs<Mword>(Cpu::rdmsr(0x486), Cpu::rdmsr(0x487));
  cr4_defs = Bit_defs<Mword>(Cpu::rdmsr(0x488), Cpu::rdmsr(0x489));

  max_index = Cpu::rdmsr(0x48a);

  assert ((Vmx::F_sw_guest_cr2 & 0x3ff) > max_index);
  max_index = Vmx::F_sw_guest_cr2 & 0x3ff;
  Vmx_user_info::Fo_table::check_offsets(max_index);

  if (basic & (1ULL << 55))
    {
      // do not use the true pin-based ctls because user-level then needs to
      // be aware of the fact that it has to set bits 1, 2, and 4 to default 1
      if (0) pinbased_ctls = Cpu::rdmsr(0x48d);

      procbased_ctls = Cpu::rdmsr(0x48e);
      exit_ctls = Cpu::rdmsr(0x48f);
      entry_ctls = Cpu::rdmsr(0x490);
    }

  if (0)
    dump("as read from hardware");

  pinbased_ctls.enforce(Vmx::PIB_ext_int_exit);
  pinbased_ctls.enforce(Vmx::PIB_nmi_exit);


  // currently we IO-passthrough is missing, disable I/O bitmaps and enforce
  // unconditional io exiting
  procbased_ctls.enforce(Vmx::PRB1_use_io_bitmaps, false);
  procbased_ctls.enforce(Vmx::PRB1_unconditional_io_exit);

  // virtual APIC not yet supported
  procbased_ctls.enforce(Vmx::PRB1_tpr_shadow, false);

  if (procbased_ctls.allowed(31))
    {
      procbased_ctls.enforce(31, true);

      procbased_ctls2 = Cpu::rdmsr(0x48b);
      if (procbased_ctls2.allowed(Vmx::PRB2_enable_ept))
	ept_vpid_cap = Cpu::rdmsr(0x48c);


      // we disable VPID so far, need to handle virtualize it in Fiasco,
      // as done for AMDs ASIDs
      procbased_ctls2.enforce(Vmx::PRB2_enable_vpid, false);

      // EPT only in conjunction with unrestricted guest !!!
      if (procbased_ctls2.allowed(Vmx::PRB2_enable_ept))
        {
          ept = true;
          procbased_ctls2.enforce(Vmx::PRB2_enable_ept, true);

          if (procbased_ctls2.allowed(Vmx::PRB2_unrestricted))
            {
              // unrestricted guest allows PE and PG to be 0
              cr0_defs.relax(0);  // PE
              cr0_defs.relax(31); // PG
              procbased_ctls2.enforce(Vmx::PRB2_unrestricted);
            }
          else
            {
              assert (not cr0_defs.allowed(0, false));
              assert (not cr0_defs.allowed(31, false));
            }
        }
      else
        assert (not procbased_ctls2.allowed(Vmx::PRB2_unrestricted));
    }
  else
    procbased_ctls2 = 0;

  // never automatically ack interrupts on exit
  exit_ctls.enforce(15, false);

  // host-state is 64bit or not
  exit_ctls.enforce(9, sizeof(long) > sizeof(int));

  if (!ept) // needs to be per VM
    {
      // always enable paging
      cr0_defs.enforce(31);
      // always PE
      cr0_defs.enforce(0);
      cr4_defs.enforce(4); // PSE

      // enforce PAE on 64bit, and disallow it on 32bit
      cr4_defs.enforce(5, sizeof(long) > sizeof(int));
    }

  if (0)
    dump("as modified");
}

PUBLIC
void
Vmx_info::dump(const char *tag) const
{
  printf("VMX MSRs %s:\n", tag);
  printf("basic                = %16llx\n", basic);
  pinbased_ctls.print("pinbased_ctls");
  procbased_ctls.print("procbased_ctls");
  exit_ctls.print("exit_ctls");
  entry_ctls.print("entry_ctls");
  printf("misc                 = %16llx\n", misc);
  cr0_defs.print("cr0_fixed");
  cr4_defs.print("cr4_fixed");
  procbased_ctls2.print("procbased_ctls2");
  printf("ept_vpid_cap         = %16llx\n", ept_vpid_cap);
}

PRIVATE static inline
Mword
Vmx::vmread_insn(Mword field)
{
  Mword val;
  asm volatile("vmread %1, %0" : "=r" (val) : "r" (field));
  return val;
}

PUBLIC static inline NEEDS[Vmx::vmread_insn]
template< typename T >
T
Vmx::vmread(Mword field)
{
  if (sizeof(T) <= sizeof(Mword))
    return vmread_insn(field);

  return vmread_insn(field) | ((Unsigned64)vmread_insn(field + 1) << 32);
}

PUBLIC static inline NEEDS["warn.h"]
template< typename T >
void
Vmx::vmwrite(Mword field, T value)
{
  Mword err;
  asm volatile("vmwrite %1, %2; pushf; pop %0" : "=r" (err) : "r" ((Mword)value), "r" (field));
  if (EXPECT_FALSE(err & 0x1))
    WARNX(Info, "VMX: VMfailInvalid vmwrite(0x%04lx, %llx) => %lx\n",
          field, (Unsigned64)value, err);
  else if (EXPECT_FALSE(err & 0x40))
    WARNX(Info, "VMX: VMfailValid vmwrite(0x%04lx, %llx) => %lx, insn error: 0x%x\n",
          field, (Unsigned64)value, err, vmread<Unsigned32>(F_vm_instruction_error));
  if (sizeof(T) > sizeof(Mword))
    asm volatile("vmwrite %0, %1" : : "r" ((Unsigned64)value >> 32), "r" (field + 1));
}

PRIVATE
bool
Vmx::handle_bios_lock()
{
  enum
  {
    Feature_control_lock            = 1 << 0,
    Feature_control_vmx_outside_SMX = 1 << 2,
  };

  Unsigned64 feature = Cpu::rdmsr(MSR_IA32_FEATURE_CONTROL);

  if (feature & Feature_control_lock)
    {
      if (!(feature & Feature_control_vmx_outside_SMX))
        return false;
    }
  else
    Cpu::wrmsr(feature | Feature_control_vmx_outside_SMX | Feature_control_lock,
               MSR_IA32_FEATURE_CONTROL);
  return true;
}

PUBLIC
void
Vmx::pm_on_resume(Cpu_number)
{
  check (handle_bios_lock());

  // enable vmx operation
  asm volatile("vmxon %0" : :"m"(_vmxon_base_pa):);

  Mword eflags;
  // make kernel vmcs current
  asm volatile("vmptrld %1 \n\t"
	       "pushf      \n\t"
	       "pop %0     \n\t" : "=r"(eflags) : "m"(_kernel_vmcs_pa):);

  // FIXME: MUST NOT PANIC ON CPU HOTPLUG
  if (eflags & 0x41)
    panic("VMX: vmptrld: VMFailInvalid, vmcs pointer not valid\n");

}

PUBLIC
void
Vmx::pm_on_suspend(Cpu_number)
{
  Mword eflags;
  asm volatile("vmclear %1 \n\t"
	       "pushf      \n\t"
	       "pop %0     \n\t" : "=r"(eflags) : "m"(_kernel_vmcs_pa):);
  if (eflags & 0x41)
    WARN("VMX: vmclear: vmcs pointer not valid\n");
}

PUBLIC
Vmx::Vmx(Cpu_number cpu)
  : _vmx_enabled(false), _has_vpid(false)
{
  Cpu &c = Cpu::cpus.cpu(cpu);
  if (!c.online() || !c.vmx())
    {
      if (cpu == Cpu_number::boot_cpu())
        WARNX(Info, "VMX: Not supported\n");
      return;
    }

  // check whether vmx is enabled by BIOS
  if (!handle_bios_lock())
    {
      if (cpu == Cpu_number::boot_cpu())
        WARNX(Info, "VMX: CPU has VMX support but it is disabled\n");
      return;
    }

  if (cpu == Cpu_number::boot_cpu())
    printf("VMX: enabled\n");

  info.init();

  // check for EPT support
  if (cpu == Cpu_number::boot_cpu())
    {
      if (info.procbased_ctls2.allowed(PRB2_enable_ept))
        printf("VMX: EPT supported\n");
      else
        printf("VMX: EPT not available\n");
    }

  // check for vpid support
  if (info.procbased_ctls2.allowed(PRB2_enable_vpid))
    _has_vpid = true;

  c.set_cr4(c.get_cr4() | (1 << 13)); // set CR4.VMXE to 1

  // if NE bit is not set vmxon will fail
  c.set_cr0(c.get_cr0() | (1 << 5));

  enum
  {
    Vmcs_size = 0x1000, // actual size may be different
  };

  Unsigned32 vmcs_size = ((info.basic & (0x1fffULL << 32)) >> 32);

  if (vmcs_size > Vmcs_size)
    {
      WARN("VMX: VMCS size of %d bytes not supported\n", vmcs_size);
      return;
    }

  // allocate a 4kb region for kernel vmcs
  // FIXME: MUST NOT PANIC ON CPU HOTPLUG
  check(_kernel_vmcs = Kmem_alloc::allocator()->alloc(12));
  _kernel_vmcs_pa = Kmem::virt_to_phys(_kernel_vmcs);
  // clean vmcs
  memset(_kernel_vmcs, 0, vmcs_size);
  // init vmcs with revision identifier
  *(int *)_kernel_vmcs = (info.basic & 0xFFFFFFFF);

  // allocate a 4kb aligned region for VMXON
  // FIXME: MUST NOT PANIC ON CPU HOTPLUG
  check(_vmxon = Kmem_alloc::allocator()->alloc(12));

  _vmxon_base_pa = Kmem::virt_to_phys(_vmxon);

  // init vmxon region with vmcs revision identifier
  // which is stored in the lower 32 bits of MSR 0x480
  *(unsigned *)_vmxon = (info.basic & 0xFFFFFFFF);

  // enable vmx operation
  asm volatile("vmxon %0" : :"m"(_vmxon_base_pa):);
  _vmx_enabled = true;

  if (cpu == Cpu_number::boot_cpu())
    printf("VMX: initialized\n");

  Mword eflags;
  asm volatile("vmclear %1 \n\t"
	       "pushf      \n\t"
	       "pop %0     \n\t" : "=r"(eflags) : "m"(_kernel_vmcs_pa):);
  // FIXME: MUST NOT PANIC ON CPU HOTPLUG
  if (eflags & 0x41)
    panic("VMX: vmclear: VMFailInvalid, vmcs pointer not valid\n");

  // make kernel vmcs current
  asm volatile("vmptrld %1 \n\t"
	       "pushf      \n\t"
	       "pop %0     \n\t" : "=r"(eflags) : "m"(_kernel_vmcs_pa):);

  // FIXME: MUST NOT PANIC ON CPU HOTPLUG
  if (eflags & 0x41)
    panic("VMX: vmptrld: VMFailInvalid, vmcs pointer not valid\n");

  Pm_object::register_pm(cpu);

  extern char entry_sys_fast_ipc_c[];
  extern char vm_vmx_exit_vec[];

  vmwrite(F_host_es_selector, GDT_DATA_KERNEL);
  vmwrite(F_host_cs_selector, GDT_CODE_KERNEL);
  vmwrite(F_host_ss_selector, GDT_DATA_KERNEL);
  vmwrite(F_host_ds_selector, GDT_DATA_KERNEL);

  /* set FS and GS to unusable in the host state */
  vmwrite(F_host_fs_selector, 0);
  vmwrite(F_host_gs_selector, 0);

  Unsigned16 tr = c.get_tr();
  vmwrite(F_host_tr_selector, tr);

  vmwrite(F_host_tr_base, ((*c.get_gdt())[tr / 8]).base());
  vmwrite(F_host_rip, vm_vmx_exit_vec);
  vmwrite<Mword>(F_host_sysenter_cs, Gdt::gdt_code_kernel);
  vmwrite(F_host_sysenter_esp, &c.kernel_sp());
  vmwrite(F_host_sysenter_eip, entry_sys_fast_ipc_c);

  if (c.features() & FEAT_PAT && info.exit_ctls.allowed(19))
    {
      vmwrite(F_host_ia32_pat, Cpu::rdmsr(MSR_PAT));
      info.exit_ctls.enforce(19, true);
    }
  else
    {
      // We have no proper PAT support, so disallow PAT load store for
      // guest too
      info.exit_ctls.enforce(18, false);
      info.entry_ctls.enforce(14, false);
    }

  if (info.exit_ctls.allowed(21)) // Load IA32_EFER
    {
      vmwrite(F_host_ia32_efer, Cpu::rdmsr(MSR_EFER));
      info.exit_ctls.enforce(21, true);
    }
  else
    {
      // We have no EFER load for host, so disallow EFER load store for
      // guest too
      info.exit_ctls.enforce(20, false);
      info.entry_ctls.enforce(15, false);
    }

  if (info.exit_ctls.allowed(12))
    vmwrite(F_host_ia32_perf_global_ctrl, Cpu::rdmsr(0x199));
  else
    // do not allow Load IA32_PERF_GLOBAL_CTRL on entry
    info.entry_ctls.enforce(13, false);

  vmwrite(F_host_cr0, Cpu::get_cr0());
  vmwrite(F_host_cr4, Cpu::get_cr4());

  Pseudo_descriptor pseudo;
  c.get_gdt()->get(&pseudo);

  vmwrite(F_host_gdtr_base, pseudo.base());

  Idt::get(&pseudo);
  vmwrite(F_host_idtr_base, pseudo.base());

  // init static guest area stuff
  vmwrite(0x2800, ~0ULL); // link pointer
  vmwrite(F_cr3_target_cnt, 0);

  // MSR load / store disabled
  vmwrite(F_exit_msr_load_cnt, 0);
  vmwrite(F_exit_msr_store_cnt, 0);
  vmwrite(F_entry_msr_load_cnt, 0);

}

/*
 * VMCS field offset table:
 *  0h -  2h: 3 offsets for 16bit fields:
 *            0: Control fields, 1: read-only fields, 2: guest state
 *            all offsets in 64byte granules relative to the start of the VMCS
 *        3h: Reserved
 *  4h -  7h: Index shift values for 16bit, 64bit, 32bit, and natural width fields
 *  8h -  Ah: 3 offsets for 64bit fields
 *  Bh -  Fh: Reserved
 * 10h - 12h: 3 offsets for 32bit fields
 * 13h - 17h: Reserved
 * 18h - 1Ah: 3 offsets for natural width fields
 *       1Bh: Reserved
 *       1Ch: Offset of first VMCS field
 *       1Dh: Full size of VMCS fields
 * 1Eh - 1Fh: Reserved
 *
 */
unsigned char const Vmx_user_info::Fo_table::master_offsets[32] =
{
    64 / 64,  768 / 64, 1472 / 64, 0,   0, 2, 1, 2,
   128 / 64,  832 / 64, 1536 / 64, 0,   0, 0, 0, 0,
   384 / 64, 1088 / 64, 1792 / 64, 0,   0, 0, 0, 0,
   512 / 64, 1216 / 64, 1920 / 64, 0,   64 / 64, 2112 / 64, 0, 0,
};

PUBLIC inline
void
Vmx::init_vmcs_infos(void *vcpu_state) const
{
  Vmx_user_info *i = reinterpret_cast<Vmx_user_info*>((char*)vcpu_state + 0x200);
  i->basic = info.basic;
  i->pinbased = info.pinbased_ctls;
  i->procbased = info.procbased_ctls;
  i->exit = info.exit_ctls;
  i->entry = info.entry_ctls;
  i->misc = info.misc;
  i->cr0_or = info.cr0_defs.must_be_one();
  i->cr0_and = info.cr0_defs.may_be_one();
  i->cr4_or = info.cr4_defs.must_be_one();
  i->cr4_and = info.cr4_defs.may_be_one();
  i->vmcs_field_info = info.max_index;
  i->procbased2 = info.procbased_ctls2;
  i->ept_vpid_cap = info.ept_vpid_cap;
  i->pinbased_dfl1 = info.pinbased_ctls_default1;
  i->procbased_dfl1 = info.procbased_ctls_default1;
  i->exit_dfl1 = info.exit_ctls_default1;
  i->entry_dfl1 = info.entry_ctls_default1;

  Vmx_user_info::Fo_table *infos = reinterpret_cast<Vmx_user_info::Fo_table *>((char*)vcpu_state + 0x420);
  Unsigned32 *inf = reinterpret_cast<Unsigned32 *>((char*)vcpu_state + 0x410);
  inf[0] = F_sw_guest_cr2;
  infos->init();
}


PUBLIC
void *
Vmx::kernel_vmcs() const
{ return _kernel_vmcs; }

PUBLIC
Address
Vmx::kernel_vmcs_pa() const
{ return _kernel_vmcs_pa; }

PUBLIC
bool
Vmx::vmx_enabled() const
{ return _vmx_enabled; }

PUBLIC
bool
Vmx::has_vpid() const
{ return _has_vpid; }
