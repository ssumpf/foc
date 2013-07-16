INTERFACE:

#include "irq.h"
#include "kobject_helper.h"
#include "l4_buf_iter.h"

class Icu_h_base
{
public:
  enum Op
  {
    Op_bind       = 0,
    Op_unbind     = 1,
    Op_info       = 2,
    Op_msi_info   = 3,
    Op_eoi        = Irq::Op_eoi_2, // 4
    Op_unmask     = Op_eoi,
    Op_mask       = 5,
    Op_set_mode   = 6,
  };

  enum Feature
  {
    Msi_bit = 0x80000000
  };
};


template< typename REAL_ICU >
class Icu_h : public Kobject_h<REAL_ICU>, public Icu_h_base
{
protected:
  REAL_ICU const *this_icu() const
  { return nonull_static_cast<REAL_ICU const *>(this); }

  REAL_ICU *this_icu()
  { return nonull_static_cast<REAL_ICU *>(this); }
};


IMPLEMENTATION:


PROTECTED
Irq *
Icu_h_base::deref_irq(L4_msg_tag *tag, Utcb const *utcb)
{
  L4_snd_item_iter snd_items(utcb, tag->words());
  Irq *irq;

  if (!tag->items() || !snd_items.next())
    {
      *tag = Kobject_iface::commit_result(-L4_err::EInval);
      return 0;
    }

  L4_fpage bind_irq(snd_items.get()->d);
  if (EXPECT_FALSE(!bind_irq.is_objpage()))
    {
      *tag = Kobject_iface::commit_error(utcb, L4_error::Overflow);
      return 0;
    }

  register Context *const c_thread = ::current();
  register Space *const c_space = c_thread->space();
  L4_fpage::Rights irq_rights = L4_fpage::Rights(0);
  irq = Kobject::dcast<Irq*>(c_space->lookup_local(bind_irq.obj_index(), &irq_rights));

  if (!irq)
    {
      *tag = Kobject_iface::commit_result(-L4_err::EInval);
      return 0;
    }

  if (EXPECT_FALSE(!(irq_rights & L4_fpage::Rights::X())))
    {
      *tag = Kobject_iface::commit_result(-L4_err::EPerm);
      return 0;
    }

  return irq;
}


PUBLIC inline
template<typename REAL_ICU>
void
Icu_h<REAL_ICU>::icu_mask_irq(bool mask, unsigned irqnum)
{
  Irq_base *i = this_icu()->icu_get_irq(irqnum);

  if (EXPECT_FALSE(!i))
    return;

  if (mask)
    i->mask();
  else
    i->unmask();
}

PUBLIC inline
template<typename REAL_ICU>
L4_msg_tag
Icu_h<REAL_ICU>::icu_unbind_irq(unsigned irqnum)
{
  Irq_base *irq = this_icu()->icu_get_irq(irqnum);

  if (irq)
    irq->unbind();

  return Kobject_iface::commit_result(0);
}

PUBLIC inline
template<typename REAL_ICU>
L4_msg_tag
Icu_h<REAL_ICU>::icu_get_msi_info(Mword msi, Utcb *out)
{
  (void) msi;
  (void) out;
  return Kobject_iface::commit_result(-L4_err::EInval);
}

PUBLIC template< typename REAL_ICU >
inline
L4_msg_tag
Icu_h<REAL_ICU>::icu_invoke(L4_obj_ref, L4_fpage::Rights /*rights*/,
                            Syscall_frame *f,
                            Utcb const *utcb, Utcb *out)
{
  L4_msg_tag tag = f->tag();

  switch (utcb->values[0])
    {
    case Op_bind:
      if (tag.words() < 2)
	return Kobject_iface::commit_result(-L4_err::EInval);

      if (Irq *irq = deref_irq(&tag, utcb))
	return this_icu()->icu_bind_irq(irq, utcb->values[1]);
      else
	return tag;

    case Op_unbind:
      if (tag.words() < 2)
	return Kobject_iface::commit_result(-L4_err::EInval);

      if (deref_irq(&tag, utcb))
	return this_icu()->icu_unbind_irq(utcb->values[1]);
      else
	return tag;

    case Op_info:
      this_icu()->icu_get_info(&out->values[0], &out->values[1], &out->values[2]);
      return Kobject_iface::commit_result(0, 3);

    case Op_msi_info:
      if (tag.words() < 2)
	return Kobject_iface::commit_result(-L4_err::EInval);
      return this_icu()->icu_get_msi_info(utcb->values[1], out);

    case Op_unmask:
    case Op_mask:
      if (tag.words() < 2)
	return Kobject_h<REAL_ICU>::no_reply();

      this_icu()->icu_mask_irq(utcb->values[0] == Op_mask, utcb->values[1]);
      return Kobject_h<REAL_ICU>::no_reply();

    case Op_set_mode:
      if (tag.words() >= 3)
        return this_icu()->icu_set_mode(utcb->values[1],
                                        Irq_chip::Mode(utcb->values[2]));
      return Kobject_iface::commit_result(-L4_err::EInval);

    default:
      return Kobject_iface::commit_result(-L4_err::ENosys);
    }
}

PUBLIC
template< typename REAL_ICU >
L4_msg_tag
Icu_h<REAL_ICU>::kinvoke(L4_obj_ref ref, L4_fpage::Rights rights,
                         Syscall_frame *f,
                         Utcb const *in, Utcb *out)
{
  L4_msg_tag tag = f->tag();

  if (EXPECT_FALSE(tag.proto() != L4_msg_tag::Label_irq))
    return Kobject_iface::commit_result(-L4_err::EBadproto);

  return icu_invoke(ref, rights, f, in, out);
}

