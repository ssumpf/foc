INTERFACE:

#include "icu_helper.h"

class Irq;

class Vlog : public Icu_h<Vlog>, public Irq_chip_soft
{
  FIASCO_DECLARE_KOBJ();
public:
  enum O_flags
  {
    F_ONLCR  = 000004, ///< Map NL to CR-NL on output
    F_OCRNL  = 000010, ///< Map CR to NL on output
    F_ONLRET = 000040, ///< Do not ouput CR
  };

  enum I_flags
  {
    F_INLCR = 000100,
    F_IGNCR = 000200,
    F_ICRNL = 000400,
  };

  enum L_flags
  {
    F_ECHO = 000010,
  };

private:
  Irq_base *_irq;
  Mword _i_flags;
  Mword _o_flags;
  Mword _l_flags;
};

IMPLEMENTATION:

#include "entry_frame.h"
#include "map_util.h"
#include "mem_space.h"
#include "l4_buf_iter.h"
#include "thread.h"
#include "vkey.h"
#include "irq.h"
#include "irq_controller.h"


FIASCO_DEFINE_KOBJ(Vlog);

PUBLIC
Vlog::Vlog()
: _irq(0),
  _i_flags(F_ICRNL), _o_flags(F_ONLCR), _l_flags(0)
{
  // CAP idx 5 is the initial kernel stream
  initial_kobjects.register_obj(this, 5);
}

PUBLIC void
Vlog::operator delete (void *)
{
  printf("WARNING: tried to delete kernel Vlog object.\n");
}


PRIVATE inline NOEXPORT
void
Vlog::log_string(Syscall_frame *f, Utcb const *u)
{
  L4_snd_item_iter snd_items(u, f->tag().words());

  unsigned len = u->values[1];
  char const *str = (char const *)&u->values[2];

  if (len > sizeof(u->values) - sizeof(u->values[0]) * 2)
    return;

  while (len--)
    {
      int c = *str++;

      // the kernel does this anyway
#if 0
      if (_o_flags & F_ONLCR && c == '\n')
	putchar('\r');
#endif

      if (_o_flags & F_OCRNL && c == '\r')
	c = '\n';

      if (_o_flags & F_ONLRET && c == '\r')
	continue;

      putchar(c);
    }
}

PRIVATE inline NOEXPORT
L4_msg_tag
Vlog::get_input(Mword rights, Syscall_frame *f, Utcb *u)
{
  (void)f;

  if (!have_receive(u))
    return commit_result(0);

  if (!(rights & L4_fpage::X))
    return commit_result(-L4_err::EPerm);

  char *buffer = reinterpret_cast<char *>(&u->values[1]);
  long cnt_down = min<Mword>(u->values[0] >> 16,
                             sizeof(u->values) - sizeof(u->values[0]));
  int i = 0;
  while (cnt_down && (i = Vkey::get()) != -1)
    {
      Vkey::clear();

      if (_i_flags & F_INLCR && i == '\n')
	i = '\r';

      if (_i_flags & F_IGNCR && i == '\r')
	continue;

      if (_i_flags & F_ICRNL && i == '\r')
	i = '\n';

      *buffer = i;
      ++buffer;
      --cnt_down;
    }

  u->values[0] = buffer - reinterpret_cast<char *>(&u->values[1]);
  if (i == -1)
    u->values[0] |= 1UL<<31;
  return commit_result(0);
}

PUBLIC
void
Vlog::bind(Irq_base *irq, Mword irqnum)
{
  Irq_chip_soft::bind(irq, irqnum);
  _irq = irq;
  Vkey::irq(irq);
}

PUBLIC
L4_msg_tag
Vlog::icu_bind_irq(Irq *irq_o, unsigned irqnum)
{
  if (irqnum > 0)
    return commit_result(-L4_err::EInval);

  if (_irq)
    _irq->unbind();

  bind(irq_o, irqnum);
  return commit_result(0);
}


PRIVATE inline NOEXPORT
L4_msg_tag
Vlog::set_attr(Mword, Syscall_frame const *, Utcb const *u)
{
  _i_flags = u->values[1];
  _o_flags = u->values[2] | F_ONLCR;
  _l_flags = u->values[3];
  Vkey::set_echo((!(_l_flags & F_ECHO))
                  ? Vkey::Echo_off
                  : (_o_flags & F_OCRNL
                    ? Vkey::Echo_crnl
                    : Vkey::Echo_on));

  return commit_result(0);
}

PRIVATE inline NOEXPORT
L4_msg_tag
Vlog::get_attr(Mword, Syscall_frame *, Utcb *u)
{
  if (!have_receive(u))
    return commit_result(0);

  u->values[1] = _i_flags;
  u->values[2] = _o_flags;
  u->values[3] = _l_flags;
  return commit_result(0, 4);
}

PUBLIC inline
Irq_base *
Vlog::icu_get_irq(unsigned irqnum)
{
  if (irqnum > 0)
    return 0;

  return _irq;
}


PUBLIC inline
void
Vlog::icu_get_info(Mword *features, Mword *num_irqs, Mword *num_msis)
{
  *features = 0; // supported features (only normal irqs)
  *num_irqs = 1;
  *num_msis = 0;
}


PUBLIC
L4_msg_tag
Vlog::kinvoke(L4_obj_ref ref, Mword rights, Syscall_frame *f,
              Utcb const *r_msg, Utcb *s_msg)
{
  L4_msg_tag const t = f->tag();

  if (t.proto() == L4_msg_tag::Label_irq)
    return Icu_h<Vlog>::icu_invoke(ref, rights, f, r_msg, s_msg);
  else if (t.proto() != L4_msg_tag::Label_log)
    return commit_result(-L4_err::EBadproto);

  switch (r_msg->values[0])
    {
    case 0:
      log_string(f, r_msg);
      return no_reply();

    case 2: // set attr
      return set_attr(rights, f, r_msg);

    case 3: // get attr
      return get_attr(rights, f, s_msg);

    default:
      return get_input(rights, f, s_msg);
    }
}


static Vlog __vlog;

