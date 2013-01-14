INTERFACE:

#include "kobject.h"
#include "thread.h"
#include <type_traits>

class Kobject_helper_base
{
protected:
  static Mword _utcb_dummy[];
  static Utcb *utcb_dummy()
  {
    char *x = reinterpret_cast<char*>(&_utcb_dummy);
    return reinterpret_cast<Utcb*>(x);
  }

  static L4_msg_tag no_reply() { return L4_msg_tag(L4_msg_tag::Schedule); }
  static bool have_receive(Utcb *rcv) { return rcv != utcb_dummy(); }
};

template<typename T, typename Base = Kobject>
class Kobject_h : public Base, protected Kobject_helper_base
{
private:
  static Sender *_sender(Thread *, Sender *t) { return t; }
  static Sender *_sender(Thread *ct, ...) { return ct; }

public:

  explicit Kobject_h() {}

  template< typename... A >
  explicit Kobject_h(A&&... args) : Base(cxx::forward<A>(args)...) {}

  void invoke(L4_obj_ref self, Mword rights, Syscall_frame *f, Utcb *u)
  {
    L4_msg_tag res(no_reply());
    if (EXPECT_TRUE(self.op() & L4_obj_ref::Ipc_send))
      res = static_cast<T*>(this)->T::kinvoke(self, rights, f, (Utcb const *)u,
                                              self.have_recv() ? u : utcb_dummy());

    if (EXPECT_FALSE(res.has_error()))
      {
	f->tag(res);
	return;
      }

    if (self.have_recv())
      {
	if (!res.do_switch())
	  {
	    Thread *t = current_thread();
	    Sender *s = (self.op() & L4_obj_ref::Ipc_open_wait) ? 0 : _sender(t, static_cast<T*>(this));
	    t->do_ipc(f->tag(), 0, 0, true, s, f->timeout(), f, rights);
	    return;
	  }
	else
	  f->tag(res);
      }
  }

};


IMPLEMENTATION:

Mword Kobject_helper_base::_utcb_dummy[(sizeof(Utcb) + sizeof(Mword) - 1) / sizeof(Mword)];

