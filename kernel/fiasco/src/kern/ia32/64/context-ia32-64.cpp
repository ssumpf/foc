INTERFACE [amd64]:

EXTENSION class Context
{
protected:
  Mword _gs_base, _fs_base;
  Unsigned32 _ds;

public:
  Mword fs_base() const { return _fs_base; }
  Mword gs_base() const { return _gs_base; }
};

// ------------------------------------------------------------------------
IMPLEMENTATION [amd64]:

IMPLEMENT inline NEEDS [Context::update_consumed_time,
			Context::store_segments]
void
Context::switch_cpu(Context *t)
{
  Mword dummy1, dummy2, dummy3, dummy4;

  update_consumed_time();

  t->load_gdt_user_entries(this);

  _ds = Cpu::get_ds();
  if (EXPECT_FALSE(_ds | t->_ds))
    Cpu::set_ds(t->_ds);

  _es = Cpu::get_es();
  if (EXPECT_FALSE(_es | t->_es))
    Cpu::set_es(t->_es);

  _fs = Cpu::get_fs();
  if (EXPECT_FALSE(_fs | _fs_base | t->_fs))
    Cpu::set_fs(t->_fs);

  if (!t->_fs)
    Cpu::wrmsr(t->_fs_base, MSR_FS_BASE);

  _gs = Cpu::get_gs();
  if (EXPECT_FALSE(_gs | _gs_base | t->_gs))
    Cpu::set_gs(t->_gs);

  if (!t->_gs)
    Cpu::wrmsr(t->_gs_base, MSR_GS_BASE);

  store_segments();
  asm volatile
    (
     "   push %%rbp			\n\t"	// save base ptr of old thread
     "   pushq $1f			\n\t"	// push restart addr on old stack
     "   mov  %%rsp, (%[old_ksp])	\n\t"	// save stack pointer
     "   mov  (%[new_ksp]), %%rsp	\n\t"	// load new stack pointer
     // in new context now (cli'd)
     "   call  switchin_context_label	\n\t"	// switch pagetable
     "   pop   %%rax			\n\t"	// don't do ret here -- we want
     "   jmp   *%%rax			\n\t"	// to preserve the return stack
     // restart code
     "  .p2align 4			\n\t"	// start code at new cache line
     "1: pop %%rbp			\n\t"	// restore base ptr

     : "=c" (dummy1), "=a" (dummy2), "=D" (dummy3), "=S" (dummy4)
     : [old_ksp]    "c" (&_kernel_sp),
       [new_ksp]    "a" (&t->_kernel_sp),
       [new_thread] "D" (t),
       [old_thread] "S" (this)
     : "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rbx", "rdx", "memory");
}

PROTECTED inline
void
Context::arch_setup_utcb_ptr()
{
  _utcb.access()->utcb_addr = (Mword)_utcb.usr().get();
  _gs_base = (Address)&_utcb.usr()->utcb_addr;
}

PROTECTED inline
void
Context::load_segments()
{
}

PROTECTED inline
void
Context::store_segments()
{}

IMPLEMENT inline
void
Context::vcpu_pv_switch_to_kernel(Vcpu_state *vcpu, bool current)
{
  _fs_base = access_once(&vcpu->host_fs_base);
  _gs_base = access_once(&vcpu->host_gs_base);

  if (current)
    {
      _ds = Cpu::get_ds();
      _es = Cpu::get_es();
      _fs = Cpu::get_fs();
      _gs = Cpu::get_gs();
    }

  vcpu->user_ds = _ds;
  vcpu->user_es = _es;
  vcpu->user_fs = _fs;
  vcpu->user_gs = _gs;

  unsigned tmp = access_once(&vcpu->host_ds);
  if (EXPECT_FALSE(current && (_ds | tmp)))
    Cpu::set_ds(tmp);
  _ds = tmp;

  tmp = access_once(&vcpu->host_es);
  if (EXPECT_FALSE(current && (_es | tmp)))
    Cpu::set_es(tmp);
  _es = tmp;

  tmp = access_once(&vcpu->host_fs);
  if (EXPECT_FALSE(current && (_fs | tmp)))
    Cpu::set_fs(tmp);
  _fs = tmp;

  if (EXPECT_TRUE(current && !tmp))
    Cpu::wrmsr(_fs_base, MSR_FS_BASE);

  tmp = access_once(&vcpu->host_gs);
  if (EXPECT_FALSE(current && (_gs | tmp)))
    Cpu::set_gs(tmp);
  _gs = tmp;

  if (EXPECT_TRUE(current && !tmp))
    Cpu::wrmsr(_gs_base, MSR_GS_BASE);
}

IMPLEMENT inline
void
Context::vcpu_pv_switch_to_user(Vcpu_state *vcpu, bool current)
{
  _fs_base = access_once(&vcpu->user_fs_base);
  _gs_base = access_once(&vcpu->user_gs_base);

  unsigned tmp = access_once(&vcpu->user_ds);
  if (EXPECT_FALSE(current && (_ds | tmp)))
    Cpu::set_ds(tmp);
  _ds = tmp;

  tmp = access_once(&vcpu->user_es);
  if (EXPECT_FALSE(current && (_es | tmp)))
    Cpu::set_es(tmp);
  _es = tmp;

  tmp = access_once(&vcpu->user_fs);
  if (EXPECT_FALSE(current && (_fs | tmp)))
    Cpu::set_fs(tmp);
  _fs = tmp;

  if (EXPECT_TRUE(current && !tmp))
    Cpu::wrmsr(_fs_base, MSR_FS_BASE);

  tmp = access_once(&vcpu->user_gs);
  if (EXPECT_FALSE(current && (_gs | tmp)))
    Cpu::set_gs(tmp);
  _gs = tmp;

  if (EXPECT_TRUE(current && !tmp))
    Cpu::wrmsr(_gs_base, MSR_GS_BASE);
}
