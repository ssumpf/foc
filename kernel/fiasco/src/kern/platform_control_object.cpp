IMPLEMENTATION:
#include "kobject_helper.h"
#include "platform_control.h"

namespace {

class Pfc : public Kobject_h<Pfc>
{
  FIASCO_DECLARE_KOBJ();
  enum class Op
  {
    Suspend_cpu    = 0x0,
    Resume_cpu     = 0x1,
    Suspend_system = 0x2,
  };

  Pfc() { initial_kobjects.register_obj(this, 8); }

  L4_msg_tag sys_suspend_cpu(unsigned char /*rights*/, Syscall_frame *f,
                             Utcb const *utcb)
  {
    if (!Platform_control::cpu_offline_available())
      return commit_result(-L4_err::ENosys);

    if (f->tag().words() < 2)
      return commit_result(-L4_err::EInval);

    unsigned cpu = utcb->values[1];

    if (cpu >= Config::Max_num_cpus || !Per_cpu_data::valid(cpu))
      return commit_result(-L4_err::EInval);

    if (!Cpu::online(cpu))
      return commit_result(-L4_err::EInval);

    Platform_control::suspend_cpu(cpu);
    return commit_result(0);
  }

  L4_msg_tag
  sys_resume_cpu(unsigned char /*rights*/, Syscall_frame *f,
                 Utcb const *utcb)
  {
    if (!Platform_control::cpu_offline_available())
      return commit_result(-L4_err::ENosys);

    if (f->tag().words() < 2)
      return commit_result(-L4_err::EInval);

    unsigned cpu = utcb->values[1];

    if (cpu >= Config::Max_num_cpus || !Per_cpu_data::valid(cpu))
      return commit_result(-L4_err::EInval);

    if (Cpu::online(cpu))
      return commit_result(-L4_err::EInval);

    int r = Platform_control::resume_cpu(cpu);
    return commit_result(r);
  }

  L4_msg_tag
  sys_suspend_system(unsigned char, Syscall_frame *, Utcb const *)
  {
    unsigned c = 0;
    for (unsigned cpu = 0; cpu < Config::Max_num_cpus; ++cpu)
      c += Cpu::online(cpu) ? 1 : 0;
    if (c > 1)
      return commit_result(-L4_err::EBusy);

    return commit_result(Platform_control::system_suspend());
  }

  static Pfc pfc;

public:
  L4_msg_tag kinvoke(L4_obj_ref, Mword rights, Syscall_frame *f,
                     Utcb const *iutcb, Utcb *)
  {
#if 0
    switch (f->tag().proto())
      {
      case L4_msg_tag::Label_scheduler:
        break;
      default:
        return commit_result(-L4_err::EBadproto);
      }
#endif

    switch (static_cast<Op>(iutcb->values[0]))
      {
      case Op::Suspend_cpu:    return sys_suspend_cpu(rights, f, iutcb);
      case Op::Resume_cpu:     return sys_resume_cpu(rights, f, iutcb);
      case Op::Suspend_system: return sys_suspend_system(rights, f, iutcb);
      default:                 return commit_result(-L4_err::ENosys);
      }
  }

};

FIASCO_DEFINE_KOBJ(Pfc);

Pfc Pfc::pfc;

}
