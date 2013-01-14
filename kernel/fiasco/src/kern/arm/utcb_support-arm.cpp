// ------------------------------------------------------------------------
IMPLEMENTATION [arm && !armv6plus]:

#include "mem_layout.h"

IMPLEMENT inline NEEDS["mem_layout.h"]
User<Utcb>::Ptr
Utcb_support::current()
{ return *reinterpret_cast<User<Utcb>::Ptr*>(Mem_layout::Utcb_ptr_page); }

IMPLEMENT inline NEEDS["mem_layout.h"]
void
Utcb_support::current(User<Utcb>::Ptr const &utcb)
{ *reinterpret_cast<User<Utcb>::Ptr*>(Mem_layout::Utcb_ptr_page) = utcb; }

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && armv6plus]:

IMPLEMENT inline
User<Utcb>::Ptr
Utcb_support::current()
{
  Utcb *u;
  asm volatile ("mrc p15, 0, %0, c13, c0, 3" : "=r" (u));
  return User<Utcb>::Ptr(u);
}

IMPLEMENT inline
void
Utcb_support::current(User<Utcb>::Ptr const &)
{}
