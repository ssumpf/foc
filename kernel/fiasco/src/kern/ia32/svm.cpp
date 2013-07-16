INTERFACE:

class Svm
{
};

//-----------------------------------------------------------------------------
INTERFACE[svm]:

#include "per_cpu_data.h"
#include "virt.h"
#include "cpu_lock.h"

EXTENSION class Svm
{
public:
  static Per_cpu<Svm> cpus;

  enum Msr_perms
  {
    Msr_intercept = 3,
    Msr_ro        = 2,
    Msr_wo        = 1,
    Msr_rw        = 0,
  };

private:
  void *_vm_hsave_area;
  void *_iopm;
  void *_msrpm;
  Unsigned32 _next_asid;
  Unsigned32 _global_asid_generation;
  Unsigned32 _max_asid;
  bool _flush_all_asids;
  bool _svm_enabled;
  bool _has_npt;
  Unsigned64 _iopm_base_pa;
  Unsigned64 _msrpm_base_pa;
  Vmcb *_kernel_vmcb;
  Address _kernel_vmcb_pa;
};

//-----------------------------------------------------------------------------
INTERFACE [svm && ia32]:

EXTENSION class Svm
{
public:
  enum { Gpregs_words = 10 };
};

//-----------------------------------------------------------------------------
INTERFACE [svm && amd64]:

EXTENSION class Svm
{
public:
  enum { Gpregs_words = 18 };
};

// -----------------------------------------------------------------------
IMPLEMENTATION[svm]:

#include "cpu.h"
#include "kmem.h"
#include "l4_types.h"
#include "warn.h"
#include <cstring>

DEFINE_PER_CPU Per_cpu<Svm> Svm::cpus(true);

PUBLIC
Svm::Svm(Cpu_number cpu)
{
  Cpu &c = Cpu::cpus.cpu(cpu);
  _svm_enabled = false;
  _next_asid = 1;
  _global_asid_generation = 0;
  _max_asid = 0;
  _flush_all_asids = true;
  _has_npt = false;

  if (!c.svm())
    return;

  Unsigned64 efer, vmcr;

  vmcr = c.rdmsr(MSR_VM_CR);
  if (vmcr & (1 << 4)) // VM_CR.SVMDIS
    {
      printf("SVM supported but locked.\n");
      return;
    }

  printf("Enabling SVM support\n");

  efer = c.rdmsr(MSR_EFER);
  efer |= 1 << 12;
  c.wrmsr(efer, MSR_EFER);

  Unsigned32 eax, ebx, ecx, edx;
  c.cpuid (0x8000000a, &eax, &ebx, &ecx, &edx);
  if (edx & 1)
    {
      printf("Nested Paging supported\n");
      _has_npt = true;
    }
  printf("NASID: 0x%x\n", ebx);
  _max_asid = ebx - 1;

  // FIXME: MUST NOT PANIC ON CPU HOTPLUG
  assert(_max_asid > 0);

  enum
  {
    Vmcb_size   = 0x1000,
    Io_pm_size  = 0x3000,
    Msr_pm_size = 0x2000,
    State_save_area_size = 0x1000,
  };

  /* 16kB IO permission map and Vmcb (16kB are good for the buddy allocator)*/
  // FIXME: MUST NOT PANIC ON CPU HOTPLUG
  check(_iopm = Kmem_alloc::allocator()->unaligned_alloc(Io_pm_size + Vmcb_size));
  _iopm_base_pa = Kmem::virt_to_phys(_iopm);
  _kernel_vmcb = (Vmcb*)((char*)_iopm + Io_pm_size);
  _kernel_vmcb_pa = Kmem::virt_to_phys(_kernel_vmcb);
  _svm_enabled = true;

  /* disbale all ports */
  memset(_iopm, ~0, Io_pm_size);

  /* clean out vmcb */
  memset(_kernel_vmcb, 0, Vmcb_size);

  /* 8kB MSR permission map */
  // FIXME: MUST NOT PANIC ON CPU HOTPLUG
  check(_msrpm = Kmem_alloc::allocator()->unaligned_alloc(Msr_pm_size));
  _msrpm_base_pa = Kmem::virt_to_phys(_msrpm);
  memset(_msrpm, ~0, Msr_pm_size);

  // allow the sysenter MSRs for the guests
  set_msr_perm(MSR_SYSENTER_CS, Msr_rw);
  set_msr_perm(MSR_SYSENTER_EIP, Msr_rw);
  set_msr_perm(MSR_SYSENTER_ESP, Msr_rw);

  /* 4kB Host state-safe area */
  // FIXME: MUST NOT PANIC ON CPU HOTPLUG
  check(_vm_hsave_area = Kmem_alloc::allocator()->unaligned_alloc(State_save_area_size));
  Unsigned64 vm_hsave_pa = Kmem::virt_to_phys(_vm_hsave_area);

  c.wrmsr(vm_hsave_pa, MSR_VM_HSAVE_PA);
}

PUBLIC
void
Svm::set_msr_perm(Unsigned32 msr, Msr_perms perms)
{
  unsigned offs;
  if (msr <= 0x1fff)
    offs = 0;
  else if (0xc0000000 <= msr && msr <= 0xc0001fff)
    offs = 0x800;
  else if (0xc0010000 <= msr && msr <= 0xc0011fff)
    offs = 0x1000;
  else
    {
      WARN("Illegal MSR %x\n", msr);
      return;
    }

  msr &= 0x1fff;
  offs += msr / 4;

  unsigned char *pm = (unsigned char *)_msrpm;

  unsigned shift = (msr & 3) * 2;
  pm[offs] = (pm[offs] & ~(3 << shift)) | ((unsigned char)perms << shift);
}

PUBLIC
Unsigned64
Svm::iopm_base_pa()
{ return _iopm_base_pa; }

PUBLIC
Unsigned64
Svm::msrpm_base_pa()
{ return _msrpm_base_pa; }

PUBLIC
Vmcb *
Svm::kernel_vmcb()
{ return _kernel_vmcb; }

PUBLIC
Address
Svm::kernel_vmcb_pa()
{ return _kernel_vmcb_pa; }

PUBLIC
bool
Svm::svm_enabled()
{ return _svm_enabled; }

PUBLIC
bool
Svm::has_npt()
{ return _has_npt; }

PUBLIC
bool
Svm::asid_valid (Unsigned32 asid, Unsigned32 generation)
{
  return ((asid > 0) &&
          (asid <= _max_asid) &&
          (generation <= _global_asid_generation));
}

PUBLIC
bool
Svm::flush_all_asids()
{ return _flush_all_asids; }

PUBLIC
void
Svm::flush_all_asids(bool val)
{ _flush_all_asids = val; }

PUBLIC
Unsigned32
Svm::global_asid_generation()
{ return _global_asid_generation; }

PUBLIC
Unsigned32
Svm::next_asid ()
{
  assert(cpu_lock.test());
  _flush_all_asids = false;
  if (_next_asid > _max_asid)
    {
      _global_asid_generation++;
      _next_asid = 1;
      // FIXME: must not crash on an overrun
      assert (_global_asid_generation < ~0U);
      _flush_all_asids = true;
    }
  return _next_asid++;
}
