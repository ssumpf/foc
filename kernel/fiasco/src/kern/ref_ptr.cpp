INTERFACE:

#include "kdb_ke.h"
#include "context.h"
#include "cpu_lock.h"
#include "globals.h"


template<typename T>
class Ref_ptr
{
public:
  Ref_ptr(T *o = 0) : _p(o)
  { __take_ref(); }

  T *ptr() const
  {
    // assert_kdb (cpu_lock.test());
    return _p;
  }

  ~Ref_ptr()
  { __drop_ref(); }

  Ref_ptr(Ref_ptr<T> const &o)
  {
    __drop_ref();
    _p = o._p;
    __take_ref();
  }

  void operator = (Ref_ptr<T> const &o)
  {
    __drop_ref();
    _p = o._p;
    __take_ref();
  }
  
  void operator = (T *o)
  {
    __drop_ref();
    _p = o;
    __take_ref();
  }

  T *operator -> () const { return _p; }

  struct Null_type;

  operator Null_type const * () const
  { return reinterpret_cast<Null_type const *>(_p); }

private:
  void __drop_ref()
  {
    if (_p && (_p->dec_ref() == 0))
      {
	current()->rcu_wait();
        delete _p;
      }
  }

  void __take_ref()
  {
    if (_p)
      _p->inc_ref();
  }

  T *_p;
};
