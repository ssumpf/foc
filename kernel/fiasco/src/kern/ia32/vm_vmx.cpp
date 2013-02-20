INTERFACE [vmx]:

#include "config.h"
#include "per_cpu_data.h"
#include "vm.h"
#include "vmx.h"

class Vmcs;

class Vm_vmx : public Vm
{
private:
  static unsigned long resume_vm_vmx(Vcpu_state *regs)
    asm("resume_vm_vmx") __attribute__((__regparm__(3)));

  enum
  {
    EFER_LME = 1 << 8,
    EFER_LMA = 1 << 10,
  };

};

//----------------------------------------------------------------------------
IMPLEMENTATION [vmx]:

#include "context.h"
#include "mem_space.h"
#include "fpu.h"
#include "ref_ptr.h"
#include "thread.h" // XXX: circular dep, move this out here!
#include "thread_state.h" // XXX: circular dep, move this out here!
#include "virt.h"
#include "idt.h"


PUBLIC
Vm_vmx::Vm_vmx(Ram_quota *q)
  : Vm(q)
{}

PUBLIC inline
void *
Vm_vmx::operator new (size_t size, void *p) throw()
{
  (void)size;
  assert (size == sizeof (Vm_vmx));
  return p;
}

PUBLIC
void
Vm_vmx::operator delete (void *ptr)
{
  Vm_vmx *t = reinterpret_cast<Vm_vmx*>(ptr);
  allocator<Vm_vmx>()->q_free(t->ram_quota(), ptr);
}




PRIVATE static inline
void *
Vm_vmx::field_offset(void *vmcs, unsigned field)
{
  return (void *)((char *)vmcs
                          + ((field >> 13) * 4 + ((field >> 10) & 3) + 1) * 0x80);
}

PRIVATE static inline
unsigned
Vm_vmx::field_width(unsigned field)
{
  static const char widths[4] = { 2, 8, 4, sizeof(Mword) };
  return widths[field >> 13];
}


PRIVATE inline
template<typename T>
Vmx_info::Flags<T>
Vm_vmx::load(unsigned field, void *vmcs, Vmx_info::Bit_defs<T> const &m)
{
  T res = m.apply(read<T>(vmcs, field));
  Vmx::vmwrite(field, res);
  return Vmx_info::Flags<T>(res);
}

PRIVATE inline
void
Vm_vmx::load(unsigned field_first, unsigned field_last, void *vmcs)
{
  for (; field_first <= field_last; field_first += 2)
    load(field_first, vmcs);
}

PRIVATE inline static
template< typename T >
T
Vm_vmx::_internal_read(void *vmcs, unsigned field)
{
  vmcs = field_offset(vmcs, field);
  return *((T *)vmcs + ((field >> 1) & 0xff));
}

PRIVATE inline static
template< typename T >
void
Vm_vmx::_internal_write(void *vmcs, unsigned field, T value)
{
  vmcs = field_offset(vmcs, field);
  *((T*)vmcs + ((field >> 1) & 0xff)) = value;
}

PRIVATE inline
void
Vm_vmx::load(unsigned field, void *vmcs)
{
  switch (field >> 13)
    {
    case 0: Vmx::vmwrite(field, _internal_read<Unsigned16>(vmcs, field)); break;
    case 1: Vmx::vmwrite(field, _internal_read<Unsigned64>(vmcs, field)); break;
    case 2: Vmx::vmwrite(field, _internal_read<Unsigned32>(vmcs, field)); break;
    case 3: Vmx::vmwrite(field, _internal_read<Mword>(vmcs, field)); break;
    }
}

PRIVATE inline
void
Vm_vmx::store(unsigned field, void *vmcs)
{
  switch (field >> 13)
    {
    case 0: _internal_write(vmcs, field, Vmx::vmread<Unsigned16>(field)); break;
    case 1: _internal_write(vmcs, field, Vmx::vmread<Unsigned64>(field)); break;
    case 2: _internal_write(vmcs, field, Vmx::vmread<Unsigned32>(field)); break;
    case 3: _internal_write(vmcs, field, Vmx::vmread<Mword>(field)); break;
    }
}

PRIVATE inline
void
Vm_vmx::store(unsigned field_first, unsigned field_last, void *vmcs)
{
  for (; field_first <= field_last; field_first += 2)
    store(field_first, vmcs);
}

PRIVATE inline static
template< typename T >
void
Vm_vmx::write(void *vmcs, unsigned field, T value)
{
  switch (field >> 13)
    {
    case 0: _internal_write(vmcs, field, (Unsigned16)value); break;
    case 1: _internal_write(vmcs, field, (Unsigned64)value); break;
    case 2: _internal_write(vmcs, field, (Unsigned32)value); break;
    case 3: _internal_write(vmcs, field, (Mword)value); break;
    }
}

PRIVATE inline static
template< typename T >
T
Vm_vmx::read(void *vmcs, unsigned field)
{
  switch (field >> 13)
    {
    case 0: return _internal_read<Unsigned16>(vmcs, field);
    case 1: return _internal_read<Unsigned64>(vmcs, field);
    case 2: return _internal_read<Unsigned32>(vmcs, field);
    case 3: return _internal_read<Mword>(vmcs, field);
    }
  return 0;
}


PRIVATE
void
Vm_vmx::load_guest_state(unsigned cpu, void *src)
{
  Vmx &vmx = Vmx::cpus.cpu(cpu);

  // read VM-entry controls, apply filter and keep for later
  Vmx_info::Flags<Unsigned32> entry_ctls
    = load<Unsigned32>(Vmx::F_entry_ctls, src, vmx.info.entry_ctls);

  Vmx_info::Flags<Unsigned32> pinbased_ctls
    = load<Unsigned32>(Vmx::F_pin_based_ctls, src, vmx.info.pinbased_ctls);

  Vmx_info::Flags<Unsigned32> procbased_ctls
    = load<Unsigned32>(Vmx::F_proc_based_ctls, src, vmx.info.procbased_ctls);

  Vmx_info::Flags<Unsigned32> procbased_ctls_2;
  if (procbased_ctls.test(Vmx::PRB1_enable_proc_based_ctls_2))
    procbased_ctls_2 = load<Unsigned32>(Vmx::F_proc_based_ctls_2, src, vmx.info.procbased_ctls2);
  else
    procbased_ctls_2 = Vmx_info::Flags<Unsigned32>(0);

  load<Unsigned32>(Vmx::F_exit_ctls, src, vmx.info.exit_ctls);

  // write 16-bit fields
  load(0x800, 0x80e, src);

  // write 64-bit fields
  load(0x2802, src);

  // check if the following bits are allowed to be set in entry_ctls
  if (entry_ctls.test(14)) // PAT load requested
    load(0x2804, src);

  if (entry_ctls.test(15)) // EFER load requested
    load(0x2806, src);

  if (entry_ctls.test(13)) // IA32_PERF_GLOBAL_CTRL load requested
    load(0x2808, src);

  // this is Fiasco.OC internal state
#if 0
  if (vmx.has_ept())
    load(0x280a, 0x2810, src);
#endif

  // write 32-bit fields
  load(0x4800, 0x482a, src);

  if (pinbased_ctls.test(6)) // activate vmx-preemption timer
    load(0x482e, src);

  // write natural-width fields
  load<Mword>(0x6800, src, vmx.info.cr0_defs);

  if (sizeof(long) > sizeof(int))
    {
      if (read<Mword>(src, 0x2806) & EFER_LME)
        Vmx::vmwrite(0x6802, (Mword)phys_dir());
      else
	WARN("VMX: No, not possible\n");
    }
  else
    {
      // for 32bit we can just load the Vm pdbr
      Vmx::vmwrite(0x6802, (Mword)phys_dir());
    }

  load<Mword>(0x6804, src, vmx.info.cr4_defs);
  load(0x6806, 0x6826, src);

  // VPID must be virtualized in Fiasco
#if 0
  if (procbased_ctls_2 & Vmx::PB2_enable_vpid)
    load(Vmx::F_vpid, src);
#endif

  // currently io-bitmaps are unsupported
  // currently msr-bitmaps are unsupported

  // load(0x200C, src); for SMM virtualization
  load(Vmx::F_tsc_offset, src);

  // no virtual APIC yet, and has to be managed in kernel somehow
#if 0
  if (procbased_ctls.test(Vmx::PRB1_tpr_shadow))
    load(0x2012, src);
#endif

  if (procbased_ctls_2.test(Vmx::PRB2_virtualize_apic))
    load(Vmx::F_apic_access_addr, src);

  // exception bit map and pf error-code stuff
  load(0x4004, 0x4008, src);

  // vm entry control stuff
  Unsigned32 irq_info = read<Unsigned32>(src, Vmx::F_entry_int_info);
  if (irq_info & (1UL << 31))
    {
      // do event injection

      // load error code, if required
      if (irq_info & (1UL << 11))
	load(Vmx::F_entry_exc_error_code, src);

      // types, that require an insn length have bit 10 set (type 4, 5, and 6)
      if (irq_info & (1UL << 10))
	load(Vmx::F_entry_insn_len, src);

      Vmx::vmwrite(Vmx::F_entry_int_info, irq_info);
    }

  // hm, we have to check for sanitizing the cr0 and cr4 shadow stuff
  load(0x6000, 0x6006, src);

  // no cr3 target values supported
}


PRIVATE
void
Vm_vmx::store_guest_state(unsigned cpu, void *dest)
{
  // read 16-bit fields
  store(0x800, 0x80e, dest);

  // read 64-bit fields
  store(0x2802, dest);

  Vmx_info &vmx_info = Vmx::cpus.cpu(cpu).info;
  Vmx_info::Flags<Unsigned32> exit_ctls
    = Vmx_info::Flags<Unsigned32>(vmx_info.exit_ctls.apply(read<Unsigned32>(dest, Vmx::F_exit_ctls)));

  if (exit_ctls.test(18)) store(Vmx::F_guest_pat, dest);
  if (exit_ctls.test(20)) store(Vmx::F_guest_efer, dest);
  if (exit_ctls.test(22)) store(Vmx::F_preempt_timer, dest);

  // EPT and PAE handling missing
#if 0
  if (Vmx::cpus.cpu(cpu).has_ept())
    store(0x280a, 0x2810, dest);
#endif

  // read 32-bit fields
  store(0x4800, 0x4826, dest);

  // sysenter msr is not saved here, because we trap all msr accesses right now
  if (0)
    {
      store(0x482a, dest);
      store(0x6824, 0x6826, dest);
    }

  // read natural-width fields
  store(0x6800, dest);
  // skip cr3
  store(0x6804, 0x6822, dest);
}

PRIVATE
void
Vm_vmx::store_exit_info(unsigned cpu, void *dest)
{
  (void)cpu;
  // read 64-bit fields, that is a EPT pf thing
#if 0
  if (Vmx::cpus.cpu(cpu).has_ept())
    store(0x2400, dest);
#endif

  // clear the valid bit in Vm-entry interruption information
    {
      Unsigned32 tmp = read<Unsigned32>(dest, Vmx::F_entry_int_info);
      if (tmp & (1UL << 31))
	write(dest, Vmx::F_entry_int_info, tmp & ~((Unsigned32)1 << 31));
    }

  // read 32-bit fields
  store(0x4400, 0x440e, dest);

  // read natural-width fields
  store(0x6400, 0x640a, dest);
}

PRIVATE
void
Vm_vmx::dump(void *v, unsigned f, unsigned t)
{
  for (; f <= t; f += 2)
    printf("%04x: VMCS: %16lx   V: %16lx\n",
	   f, Vmx::vmread<Mword>(f), read<Mword>(v, f));
}

PRIVATE
void
Vm_vmx::dump_state(void *v)
{
  dump(v, 0x0800, 0x080e);
  dump(v, 0x0c00, 0x0c0c);
  dump(v, 0x2000, 0x201a);
  dump(v, 0x2800, 0x2810);
  dump(v, 0x2c00, 0x2804);
  dump(v, 0x4000, 0x4022);
  dump(v, 0x4400, 0x4420);
  dump(v, 0x4800, 0x482a);
  dump(v, 0x6800, 0x6826);
  dump(v, 0x6c00, 0x6c16);
}

PRIVATE inline NOEXPORT
int
Vm_vmx::do_resume_vcpu(Context *ctxt, Vcpu_state *vcpu, void *vmcs_s)
{
  assert (cpu_lock.test());

  /* these 4 must not use ldt entries */
  assert (!(Cpu::get_cs() & (1 << 2)));
  assert (!(Cpu::get_ss() & (1 << 2)));
  assert (!(Cpu::get_ds() & (1 << 2)));
  assert (!(Cpu::get_es() & (1 << 2)));

  unsigned cpu = current_cpu();
  Vmx &v = Vmx::cpus.cpu(cpu);

  if (!v.vmx_enabled())
    {
      WARNX(Info, "VMX: not supported/enabled\n");
      return -L4_err::ENodev;
    }

  // XXX:
  // This generates a circular dep between thread<->task, this cries for a
  // new abstraction...
  if (!(ctxt->state() & Thread_fpu_owner))
    {
      if (EXPECT_FALSE(!static_cast<Thread*>(ctxt)->switchin_fpu()))
        {
          WARN("VMX: switchin_fpu failed\n");
          return -L4_err::EInval;
        }
    }

#if 0
  if (EXPECT_FALSE(read<Unsigned32>(vmcs_s, 0x201a) != 0)) // EPT POINTER
    {
      WARN("VMX: no nested paging available\n");
      return commit_result(-L4_err::EInval);
    }
#endif

  // increment our refcount, and drop it at the end automatically
  Ref_ptr<Vm_vmx> pin_myself(this);

  // set volatile host state
  Vmx::vmwrite<Mword>(Vmx::F_host_cr3, Cpu::get_pdbr()); // host_area.cr3

  load_guest_state(cpu, vmcs_s);

  Unsigned16 ldt = Cpu::get_ldt();

  // set guest CR2
  asm volatile("mov %0, %%cr2" : : "r" (read<Mword>(vmcs_s, Vmx::F_guest_cr2)));

  unsigned long ret = resume_vm_vmx(vcpu);
  // vmread error?
  if (EXPECT_FALSE(ret & 0x40))
    return -L4_err::EInval;

  // save guest cr2
    {
      Mword cpu_cr2;
      asm volatile("mov %%cr2, %0" : "=r" (cpu_cr2));
      write(vmcs_s, Vmx::F_guest_cr2, cpu_cr2);
    }

  Cpu::set_ldt(ldt);

  // reload TSS, we use I/O bitmaps
  // ... do this lazy ...
  {
    // clear busy flag
    Gdt_entry *e = &(*Cpu::cpus.cpu(cpu).get_gdt())[Gdt::gdt_tss / 8];
    e->access &= ~(1 << 1);
    asm volatile("" : : "m" (*e));
    Cpu::set_tr(Gdt::gdt_tss);
  }

  store_guest_state(cpu, vmcs_s);
  store_exit_info(cpu, vmcs_s);

  if ((read<Unsigned32>(vmcs_s, Vmx::F_exit_reason) & 0xffff) == 1)
    return 1;

  vcpu->state &= ~(Vcpu_state::F_traps | Vcpu_state::F_user_mode);
  return 0;
}

PUBLIC
int
Vm_vmx::resume_vcpu(Context *ctxt, Vcpu_state *vcpu, bool user_mode)
{
  (void)user_mode;
  assert_kdb (user_mode);

  if (EXPECT_FALSE(!(ctxt->state(true) & Thread_ext_vcpu_enabled)))
    {
      ctxt->arch_load_vcpu_kern_state(vcpu, true);
      return -L4_err::EInval;
    }

  void *vmcs_s = reinterpret_cast<char *>(vcpu) + 0x400;

  for (;;)
    {
      // in the case of disabled IRQs and a pending IRQ directly simulate an
      // external interrupt intercept
      if (   !(vcpu->_saved_state & Vcpu_state::F_irqs)
	  && (vcpu->sticky_flags & Vcpu_state::Sf_irq_pending))
	{
	  // XXX: check if this is correct, we set external irq exit as reason
	  write<Unsigned32>(vmcs_s, Vmx::F_exit_reason, 1);
          ctxt->arch_load_vcpu_kern_state(vcpu, true);
	  return 1; // return 1 to indicate pending IRQs (IPCs)
	}

      int r = do_resume_vcpu(ctxt, vcpu, vmcs_s);

      // test for error or non-IRQ exit reason
      if (r <= 0)
        {
          ctxt->arch_load_vcpu_kern_state(vcpu, true);
          return r;
        }

      // check for IRQ exits and allow to handle the IRQ
      if (r == 1)
	Proc::preemption_point();

      // Check if the current context got a message delivered.
      // This is done by testing for a valid continuation.
      // When a continuation is set we have to directly
      // leave the kernel to not overwrite the vcpu-regs
      // with bogus state.
      Thread *t = nonull_static_cast<Thread*>(ctxt);
      if (t->continuation_test_and_restore())
        {
          ctxt->arch_load_vcpu_kern_state(vcpu, true);
          t->fast_return_to_user(vcpu->_entry_ip, vcpu->_entry_sp,
                                 t->vcpu_state().usr().get());
        }
    }
}
