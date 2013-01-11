#ifndef TYPES_H__
#define TYPES_H__

#include <stddef.h>
#include "types-arch.h"
#include "std_macros.h"

#ifdef __cplusplus

template< typename a, typename b > inline
a nonull_static_cast( b p )
{
  Address d = reinterpret_cast<Address>
                 (static_cast<a>(reinterpret_cast<b>(10))) - 10;
  return reinterpret_cast<a>( reinterpret_cast<Address>(p) + d);
}

template< typename T >
T access_once(T const &a)
{ return static_cast<T const volatile &>(a); }

template< typename T >
class Static_object
{
private:
  // prohibit copies
  Static_object(Static_object<T> const &);
  void operator = (Static_object<T> const &);

  class O : public T
  {
  public:
    void *operator new (size_t, void *p) throw() { return p; }

    O() : T() {}

    template<typename A1>
    O(A1 const &a1) : T(a1) {}

    template<typename A1, typename A2>
    O(A1 const &a1, A2 const &a2) : T(a1, a2) {}

    template<typename A1, typename A2, typename A3>
    O(A1 const &a1, A2 const &a2, A3 const &a3) : T(a1, a2, a3) {}

    template<typename A1, typename A2, typename A3, typename A4>
    O(A1 const &a1, A2 const &a2, A3 const &a3, A4 const &a4) : T(a1, a2, a3, a4) {}

    template<typename A1, typename A2, typename A3, typename A4, typename A5>
    O(A1 const &a1, A2 const &a2, A3 const &a3, A4 const &a4, A5 const &a5)
    : T(a1, a2, a3, a4, a5) {}
  };

public:
  Static_object() {}

  T *get() const
  {
    Address i = (Address)_i;
    return reinterpret_cast<O*>(i);
  }

  operator T * () const { return get(); }
  T *operator -> () const { return get(); }

  T *construct()
  { return new (_i) O(); }

  template<typename A1>
  T *construct(A1 const &a1)
  { return new (_i) O(a1); }

  template<typename A1, typename A2>
  T *construct(A1 const &a1, A2 const &a2)
  { return new (_i) O(a1, a2); }

  template<typename A1, typename A2, typename A3>
  T *construct(A1 const &a1, A2 const &a2, A3 const &a3)
  { return new (_i) O(a1, a2, a3); }

  template<typename A1, typename A2, typename A3, typename A4>
  T *construct(A1 const &a1, A2 const &a2, A3 const &a3, A4 const &a4)
  { return new (_i) O(a1, a2, a3, a4); }

  template<typename A1, typename A2, typename A3, typename A4, typename A5>
  T *construct(A1 const &a1, A2 const &a2, A3 const &a3, A4 const &a4, A5 const &a5)
  { return new (_i) O(a1, a2, a3, a4, a5); }

private:
  mutable char __attribute__((aligned(sizeof(Mword)*2))) _i[sizeof(O)];
};


template< typename VALUE, typename TARGET >
class Number
{
public:
  typedef VALUE Value;
  typedef TARGET Target;
  struct Type_conversion_error;

protected:
  Number() {}
  explicit Number(Value v) : _v(v) {}

public:

  static Target create(Value v)
  { return Target(v); }

  Target operator = (Target o)
  { _v = o._v; return *static_cast<Target*>(this); }

  Value value() const { return _v; }
  Value value() const volatile { return _v; }
  void set_value(Value v) { _v = v; }

  bool operator < (Target const &o) const { return _v < o._v; }
  bool operator > (Target const &o) const { return _v > o._v; }
  bool operator <= (Target const &o) const { return _v <= o._v; }
  bool operator >= (Target const &o) const { return _v >= o._v; }
  bool operator == (Target const &o) const { return _v == o._v; }
  bool operator != (Target const &o) const { return _v != o._v; }

  operator Type_conversion_error const * () const
  { return (Type_conversion_error const *)_v; }

  Target operator | (Target const &o) const
  { return Target(_v | o._v); }

  Target operator & (Target const &o) const
  { return Target(_v & o._v); }

  Target operator + (Target const &o) const
  { return Target(_v + o._v); }

  Target operator - (Target const &o) const
  { return Target(_v - o._v); }

  Target operator << (unsigned long s) const
  { return Target(_v << s); }

  Target operator >> (unsigned long s) const
  { return Target(_v >> s); }

  void operator += (Target const &o) { _v += o._v; }
  void operator -= (Target const &o) { _v -= o._v; }
  void operator <<= (unsigned long s) { _v <<= s; }
  void operator >>= (unsigned long s) { _v >>= s; }

  Target operator ++ () { return Target(++_v); }
  Target operator ++ (int) { return Target(_v++); }

  Target operator -- () { return Target(--_v); }
  Target operator -- (int) { return Target(_v--); }

  Target trunc(Target const &size) const
  { return Target(_v & ~(size._v - 1)); }

  Target offset(Target const &size) const
  { return Target(_v & (size._v - 1)); }

  static Target from_shift(unsigned char shift)
  {
    if (shift >= (int)sizeof(Value) * 8)
      return Target(Value(1) << Value((sizeof(Value) * 8 - 1)));
    return Target(Value(1) << Value(shift));
  }

protected:
  Value _v;
};

template< int SHIFT >
class Page_addr : public Number<Address, Page_addr<SHIFT> >
{
private:
  typedef Number<Address, Page_addr<SHIFT> > B;

  template< int T >
  class Itt
  {};

  template< int SH >
  Address __shift(Address x, Itt<true>) { return x << SH; }

  template< int SH >
  Address __shift(Address x, Itt<false>) { return x >> (-SH); }

public:
  enum { Shift = SHIFT };

  template< int OSHIFT >
  Page_addr(Page_addr<OSHIFT> o)
  : B(__shift<Shift-OSHIFT>(o.value(), Itt<(OSHIFT < Shift)>()))
  {}

  explicit Page_addr(Address a) : B(a) {}

  Page_addr(Page_addr const volatile &o) : B(o.value()) {}
  Page_addr(Page_addr const &o) : B(o.value()) {}

  Page_addr() {}
};

class Virt_addr : public Page_addr<ARCH_PAGE_SHIFT>
{
public:
  template< int OSHIFT >
  Virt_addr(Page_addr<OSHIFT> o) : Page_addr<ARCH_PAGE_SHIFT>(o) {}

  explicit Virt_addr(unsigned int a) : Page_addr<ARCH_PAGE_SHIFT>(a) {}
  explicit Virt_addr(int a) : Page_addr<ARCH_PAGE_SHIFT>(a) {}
  explicit Virt_addr(long int a) : Page_addr<ARCH_PAGE_SHIFT>(a) {}
  explicit Virt_addr(unsigned long a) : Page_addr<ARCH_PAGE_SHIFT>(a) {}
  Virt_addr(void *a) : Page_addr<ARCH_PAGE_SHIFT>(Address(a)) {}

  Virt_addr() {}
};

typedef Page_addr<ARCH_PAGE_SHIFT> Virt_size;

typedef Page_addr<0> Page_number;
typedef Page_number Page_count;

template<typename T>
struct Simple_ptr_policy
{
  typedef T &Deref_type;
  typedef T *Ptr_type;
  typedef T *Member_type;
  typedef T *Storage_type;

  static void init(Storage_type const &) {}
  static void init(Storage_type &d, Storage_type const &s) { d = s; }
  static void copy(Storage_type &d, Storage_type const &s) { d = s; }
  static void destroy(Storage_type const &) {}
  static Deref_type deref(Storage_type p) { return *p; }
  static Member_type member(Storage_type p) { return p; }
  static Ptr_type ptr(Storage_type p) { return p; }
};

template<>
struct Simple_ptr_policy<void>
{
  typedef void Deref_type;
  typedef void *Ptr_type;
  typedef void Member_type;
  typedef void *Storage_type;

  static void init(Storage_type const &) {}
  static void init(Storage_type &d, Storage_type const &s) { d = s; }
  static void copy(Storage_type &d, Storage_type const &s) { d = s; }
  static void destroy(Storage_type const &) {}
  static Deref_type deref(Storage_type p);
  static Member_type member(Storage_type p);
  static Ptr_type ptr(Storage_type p) { return p; }
};


template<typename T, template<typename P> class Policy = Simple_ptr_policy,
         typename Discriminator = int>
class Smart_ptr
{
private:
  struct Null_check_type;
public:
  typedef typename Policy<T>::Deref_type Deref_type;
  typedef typename Policy<T>::Ptr_type Ptr_type;
  typedef typename Policy<T>::Member_type Member_type;
  typedef typename Policy<T>::Storage_type Storage_type;

  template<typename A, template<typename X> class B, typename C>
  friend class Smart_ptr;

protected:
  Storage_type _p;

public:
  Smart_ptr()
  { Policy<T>::init(_p); }

  explicit Smart_ptr(T *p)
  { Policy<T>::init(_p, p); }

  Smart_ptr(Smart_ptr const &o)
  { Policy<T>::copy(_p, o._p); }

  template< typename RHT >
  Smart_ptr(Smart_ptr<RHT, Policy, Discriminator> const &o)
  { Policy<T>::copy(_p, o._p); }

  ~Smart_ptr()
  { Policy<T>::destroy(_p); }

  Smart_ptr operator = (Smart_ptr const &o)
  {
    if (this == &o)
      return *this;

    Policy<T>::destroy(_p);
    Policy<T>::copy(_p, o._p);
    return *this;
  }

  Deref_type operator * () const
  { return Policy<T>::deref(_p); }

  Member_type operator -> () const
  { return Policy<T>::member(_p); }

  Ptr_type get() const
  { return Policy<T>::ptr(_p); }

  operator Null_check_type const * () const
  { return reinterpret_cast<Null_check_type const *>(Policy<T>::ptr(_p)); }
};

enum User_ptr_discr {};

template<typename T>
struct User
{
  typedef Smart_ptr<T, Simple_ptr_policy, User_ptr_discr> Ptr;
};
#endif

/// standard size type
///typedef mword_t size_t;
///typedef signed int ssize_t;

/// momentary only used in UX since there the kernel has a different
/// address space than user mode applications
enum Address_type { ADDR_KERNEL = 0, ADDR_USER = 1, ADDR_UNKNOWN = 2 };

#endif // TYPES_H__

