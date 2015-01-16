INTERFACE:

#include "irq.h"
#include "ram_quota.h"
#include "icu_helper.h"

class Irq_chip;

class Icu : public Icu_h<Icu>
{
  FIASCO_DECLARE_KOBJ();

  friend class Icu_h<Icu>;
};


//----------------------------------------------------------------------------
IMPLEMENTATION:

#include "entry_frame.h"
#include "irq.h"
#include "irq_chip.h"
#include "irq_mgr.h"
#include "l4_types.h"
#include "l4_buf_iter.h"

FIASCO_DEFINE_KOBJ(Icu);

PUBLIC void
Icu::operator delete (void *)
{
  printf("WARNING: tried to delete kernel ICU object.\n"
         "         The system is now useless\n");
}

PUBLIC inline NEEDS["irq_mgr.h"]
Irq_base *
Icu::icu_get_irq(unsigned irqnum)
{
  return Irq_mgr::mgr->irq(irqnum);
}


PUBLIC inline NEEDS["irq_mgr.h"]
L4_msg_tag
Icu::icu_bind_irq(Irq *irq, unsigned irqnum)
{
  irq->unbind();

  if (!Irq_mgr::mgr->alloc(irq, irqnum))
    return commit_result(-L4_err::EPerm);

  return commit_result(0);
}

PUBLIC inline NEEDS["irq_mgr.h"]
L4_msg_tag
Icu::icu_set_mode(Mword pin, Irq_chip::Mode mode)
{
  Irq_mgr::Irq i = Irq_mgr::mgr->chip(pin);

  if (!i.chip)
    return commit_result(-L4_err::ENodev);

  int r = i.chip->set_mode(i.pin, mode);

  Irq_base *irq = i.chip->irq(i.pin);
  if (irq)
    irq->switch_mode(i.chip->is_edge_triggered(i.pin));

  return commit_result(r);
}


PUBLIC inline NEEDS["irq_mgr.h"]
void
Icu::icu_get_info(Mword *features, Mword *num_irqs, Mword *num_msis)
{
  *num_irqs = Irq_mgr::mgr->nr_irqs();
  *num_msis = Irq_mgr::mgr->nr_msis();
  *features = *num_msis ? (unsigned)Msi_bit : 0;
}

PUBLIC inline NEEDS["irq_mgr.h"]
L4_msg_tag
Icu::icu_get_msi_info(Mword msi, Utcb *out)
{
  out->values[0] = Irq_mgr::mgr->msg(msi);
  return commit_result(0, 1);
}


PUBLIC inline
Icu::Icu()
{
  initial_kobjects.register_obj(this, 6);
}

