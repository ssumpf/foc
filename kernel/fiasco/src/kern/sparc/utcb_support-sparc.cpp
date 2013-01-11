//-------------------------------------------------------------------------
IMPLEMENTATION [sparc]:

IMPLEMENT inline
User<Utcb>::Ptr
Utcb_support::current()
{
  Utcb *u = (Utcb*)0xDEADBEEF;
  return User<Utcb>::Ptr(u);
}

IMPLEMENT inline
void
Utcb_support::current(User<Utcb>::Ptr const &utcb)
{
  (void)utcb;
}
