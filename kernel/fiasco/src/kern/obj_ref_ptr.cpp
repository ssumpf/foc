INTERFACE:

class Ram_quota;

template< typename T >
class Ref_ptr
{
public:
  class Inv_ptr;
  Ref_ptr(Ref_ptr<T> const &o) : _o(o._o)  { inc_ref_cnt(); }
  Ref_ptr(T *o = 0) : _o(o)  { inc_ref_cnt(); }
  ~Ref_ptr() { dec_ref_cnt(); }

  void operator = (Ref_ptr<T> const &o)
  { dec_ref_cnt(); _o = o._o; inc_ref_cnt(); }

  void operator = (T *o)
  { dec_ref_cnt(); _o = o; inc_ref_cnt(); }

  T *operator -> () const { return _o; }
  T *operator * () const { return _o; }
  operator Inv_ptr const * () const 
  { return reinterpret_cast<Inv_ptr const*>(_o); }

private:
  T *_o;

  void inc_ref_cnt()
  {
    if (_o) 
      {
	Lock_guard<Cpu_lock> guard(&cpu_lock);
	_o->inc_ref_cnt();
      }
  }

  void dec_ref_cnt() const
  { 
    if (_o && _o->dec_ref_cnt() == 0)
      delete _o;
  }

};

