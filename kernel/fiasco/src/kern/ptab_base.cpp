INTERFACE:

#include "mem_layout.h"

namespace Ptab
{

  struct Null_alloc
  {
    static void *alloc(unsigned long) { return 0; }
    static void free(void *) {}
    static bool valid() { return false; }
  };

  template< typename _Head, typename _Tail >
  class List
  {
  public:
    typedef _Head Head;
    typedef _Tail Tail;
  };


  template< typename _T, unsigned _Level >
  class Level
  {
  public:
    typedef _T Traits;

    static unsigned shift(unsigned)
    { return Traits::Shift; }

    static Address addr(unsigned, Mword entry)
    {
      struct E : public Traits::Entry
      { E(Mword raw) { Traits::Entry::_raw = raw; } };
      return E(entry).addr();
    }

    static unsigned size(unsigned)
    { return Traits::Size; }

    static unsigned length(unsigned)
    { return 1UL << Traits::Size; }

    static Address index(unsigned /*level*/, Address addr)
    { return (addr >> Traits::Shift) & ((1UL << Traits::Size)-1); }

  };

  template< typename _Head, typename _Tail, unsigned _Level >
  class Level< List<_Head, _Tail>, _Level >
  {
  public:
    typedef typename Level<_Tail, _Level - 1>::Traits Traits;

    static unsigned shift(unsigned level)
    {
      if (!level)
	return _Head::Shift;
      else
	return Level<_Tail, _Level - 1>::shift(level - 1);
    }

    static Address addr(unsigned level, Mword entry)
    {
      struct E : public Traits::Entry
      { E(Mword raw) { Traits::Entry::_raw = raw; } };
      if (!level)
	return E(entry).addr();
      else
        return Level<_Tail, _Level - 1>::addr(level - 1, entry);
    }

    static unsigned size(unsigned level)
    {
      if (!level)
	return _Head::Size;
      else
	return Level<_Tail, _Level - 1>::size(level - 1);
    }

    static unsigned length(unsigned level)
    {
      if (!level)
	return 1UL << _Head::Size;
      else
	return Level<_Tail, _Level - 1>::length(level - 1);
    }

    static Address index(unsigned level, Address addr)
    {
      if (!level)
	return (addr >> Traits::Shift) & ((1UL << Traits::Size)-1);
      else
	return Level<_Tail, _Level - 1>::index(level - 1, addr);
    }

  };

  template< typename _Head, typename _Tail>
  class Level< List<_Head, _Tail>, 0> : public Level<_Head, 0>
  {
  };

  template< typename _Traits >
  class Entry_vec
  {
  public:
    typedef typename _Traits::Entry Entry;
    enum
    {
      Length = 1UL << _Traits::Size,
      Size   = _Traits::Size,
      Mask   = _Traits::Mask,
      Shift  = _Traits::Shift,
    };


    Entry _e[Length];

    static unsigned idx(Address virt)
    {
      if (Mask)
	return (virt >> Shift) & ~(~0UL << Size);
      else
	return (virt >> Shift);
    }

    Entry &operator [] (unsigned idx) { return _e[idx]; }
    Entry const &operator [] (unsigned idx) const { return _e[idx]; }

    void clear() { for (unsigned i=0; i < Length; ++i) _e[i].clear(); }
  };


  template< typename _Last, typename Iter >
  class Walk
  {
  public:
    enum { Depth = 0 };
    typedef _Last Level;
    typedef typename _Last::Entry Entry;
    typedef _Last Traits;

  private:
    typedef Walk<_Last, Iter> This;
    typedef Entry_vec<Level> Vec;
    Vec _e;

  public:
    void clear() { _e.clear(); }

    template< typename _Alloc >
    Iter walk(Address virt, unsigned, _Alloc const &)
    { return Iter(&_e[Vec::idx(virt)], Level::Shift); }

    void unmap(Address &start, unsigned long &size, unsigned)
    {
      unsigned idx = Vec::idx(start);
      unsigned cnt = size >> Traits::Shift;
      if (cnt + idx > Vec::Length)
	cnt = Vec::Length - idx;
      unsigned const e = idx + cnt;

      for (; idx != e; ++idx)
	_e[idx].clear();

      start += (unsigned long)cnt << Traits::Shift;
      size  -= (unsigned long)cnt << Traits::Shift;
    }

    template< typename _Alloc >
    void map(Address &phys, Address &virt, unsigned long &size,
	unsigned long attr, unsigned, _Alloc const &)
    {
      unsigned idx = Vec::idx(virt);
      unsigned cnt = size >> Traits::Shift;
      if (cnt + idx > Vec::Length)
	cnt = Vec::Length - idx;
      unsigned const e = idx + cnt;

      for (; idx != e; ++idx, phys += (1UL << Traits::Entry::Page_shift))
	_e[idx].set(phys, false, true, attr);
      virt += (unsigned long)cnt << Traits::Shift;
      size -= (unsigned long)cnt << Traits::Shift;
    }

    template< typename _Alloc >
    void destroy(Address, Address, unsigned, _Alloc const &)
    {}

    template< typename _Alloc >
    bool sync(Address &l_addr, This const &_r, Address &r_addr,
	Address &size, unsigned, _Alloc const &)
    {
      unsigned count = size >> Traits::Shift;
      unsigned const l = Vec::idx(l_addr);
      unsigned const r = Vec::idx(r_addr);
      unsigned const m = l > r ? l : r;
      if (m + count >= Vec::Length)
	count = Vec::Length - m;

      Entry *le = &_e[l];
      Entry const *re = &_r._e[r];

      bool need_flush = false;

      for (unsigned n = count; n > 0; --n)
	{
	  if (le[n-1].valid())
	    need_flush = true;

	  // This loop seems unnecessary, but remote_update is also used for
	  // updating the long IPC window.
	  // Now consider following scenario with super pages:
	  // Sender A makes long IPC to receiver B.
	  // A setups the IPC window by reading the pagedir slot from B in an 
	  // temporary register. Now the sender is preempted by C. Then C unmaps 
	  // the corresponding super page from B. C switch to A back, using 
	  // switch_to, which clears the IPC window pde slots from A. BUT then A 
	  // write the  content of the temporary register, which contain the now 
	  // invalid pde slot, in his own page directory and starts the long IPC.
	  // Because no pagefault will happen, A will write to now invalid memory.
	  // So we compare after storing the pde slot, if the copy is still
	  // valid. And this solution is much faster than grabbing the cpu lock,
	  // when updating the ipc window.h 
	  for (;;)
	    {
	      typename Traits::Raw const volatile *rr
		= reinterpret_cast<typename Traits::Raw const *>(re + n - 1);
	      le[n - 1] = *(Entry *)rr;
	      if (EXPECT_TRUE(le[n - 1].raw() == *rr))
		break;
	    }
	}

      l_addr += (unsigned long)count << Traits::Shift;
      r_addr += (unsigned long)count << Traits::Shift;
      size -= (unsigned long)count << Traits::Shift;
      return need_flush;
    }
  };



  template< typename _Head, typename _Tail, typename Iter >
  class Walk <List <_Head,_Tail>, Iter >
  {
  public:
    typedef Walk<_Tail, Iter> Next;
    typedef typename Next::Level Level;
    typedef typename _Head::Entry Entry;
    typedef _Head Traits;

    enum { Depth = Next::Depth + 1 };

  private:
    typedef Walk<_Head, Iter> This;
    typedef Walk< List< _Head, _Tail >, Iter> This2;
    typedef Entry_vec<_Head> Vec;
    Vec _e;

    template< typename _Alloc >
    Next *alloc_next(Entry *e, _Alloc const &a)
    {
      Next *n = (Next*)a.alloc(sizeof(Next));
      if (EXPECT_FALSE(!n))
	return 0;

      n->clear();
      e->set(Mem_layout::pmem_to_phys(n), true, true);

      return n;
    }

  public:
    void clear() { _e.clear(); }

    template< typename _Alloc >
    Iter walk(Address virt, unsigned level, _Alloc const &alloc)
    {
      Entry *e = &_e[Vec::idx(virt)];
      if (!level)
	return Iter(e, _Head::Shift);
      else if (!e->valid())
	{
	  Next *n;
	  if (alloc.valid() && (n = alloc_next(e, alloc)))
	    return n->walk(virt, level - 1, alloc);

	  return Iter(e, _Head::Shift);
	}


      if (_Head::May_be_leaf && e->leaf())
	return Iter(e, _Head::Shift);
      else
	{
	  Next *n = (Next*)Mem_layout::phys_to_pmem(e->addr());
	  return n->walk(virt, level - 1, alloc);
	}
    }

    void unmap(Address &start, unsigned long &size, unsigned level)
    {
      if (!level)
	{
	  reinterpret_cast<This*>(this)->unmap(start, size, 0);
	  return;
	}

      while (size)
	{
	  Entry *e = &_e[Vec::idx(start)];

	  if (!e->valid() || e->leaf())
	    continue;

	  Next *n = (Next*)Mem_layout::phys_to_pmem(e->addr());
	  n->unmap(start, size, level - 1);
	}
    }

    template< typename _Alloc >
    void map(Address &phys, Address &virt, unsigned long &size,
	unsigned long attr, unsigned level, _Alloc const &alloc)
    {
      if (!level)
	{
	  reinterpret_cast<This*>(this)->map(phys, virt, size, attr, 0, alloc);
	  return;
	}

      while (size)
	{
	  Entry *e = &_e[Vec::idx(virt)];
	  Next *n;
	  if (!e->valid())
	    {
	      if (alloc.valid() && (n = alloc_next(e, alloc)))
		n->map(phys, virt, size, attr, level - 1, alloc);

	      continue;
	    }

	  if (_Head::May_be_leaf && e->leaf())
	    continue;

	  n = (Next*)Mem_layout::phys_to_pmem(e->addr());
	  n->map(phys, virt, size, attr, level - 1, alloc);
	}
    }

    template< typename _Alloc >
    void destroy(Address start, Address end, unsigned level, _Alloc const &alloc)
    {
      if (!alloc.valid())
	return;

      unsigned idx_start = Vec::idx(start);
      unsigned idx_end = Vec::idx(end + (1UL << Traits::Shift) - 1);
      unsigned idx = idx_start;

      for (; idx < idx_end; ++idx)
	{
	  Entry *e = &_e[idx];
	  if (!e->valid() || (_Head::May_be_leaf && e->leaf()))
	    continue;

	  Next *n = (Next*)Mem_layout::phys_to_pmem(e->addr());
	  if (level)
	    n->destroy(idx > idx_start ? 0 : start,
	               idx + 1 < idx_end ? 1UL << Next::Traits::Shift : end,
	               level - 1, alloc);

	  alloc.free(n, sizeof(Next));
	}
    }

    template< typename _Alloc >
    bool sync(Address &l_a, This2 const &_r, Address &r_a,
	Address &size, unsigned level, _Alloc const &alloc)
    {
      if (!level)
	return reinterpret_cast<This*>(this)
	  ->sync(l_a, reinterpret_cast<This const &>(_r), r_a, size, 0, alloc);

      unsigned count = size >> Traits::Shift;
	{
	  unsigned const lx = Vec::idx(l_a);
	  unsigned const rx = Vec::idx(r_a);
	  unsigned const mx = lx > rx ? lx : rx;
	  if (mx + count >= Vec::Length)
	    count = Vec::Length - mx;
	}

      bool need_flush = false;

      for (unsigned i = count; size && i > 0; --i) //while (size)
	{
	  Entry *l = &_e[Vec::idx(l_a)];
	  Entry const *r = &_r._e[Vec::idx(r_a)];
	  Next *n = 0;
	  if (!r->valid()
	      || (!l->valid() && (!alloc.valid()
		  || !(n = alloc_next(l,alloc)))))
	    {
	      l_a += 1UL << Traits::Shift;
	      r_a += 1UL << Traits::Shift;
	      if (size > 1UL << Traits::Shift)
		{
		  size -= 1UL << Traits::Shift;
		  continue;
		}
	      break;
	    }

	  if (!n)
	    n = (Next*)Mem_layout::phys_to_pmem(l->addr());

	  Next *rn = (Next*)Mem_layout::phys_to_pmem(r->addr());

	  if (n->sync(l_a, *rn, r_a, size, level - 1, alloc))
	    need_flush = true;
	}

      return need_flush;
    }

  };

  template< typename _E, unsigned _Va_shift >
  class Iter
  {
  public:
    Iter(_E *e, unsigned char shift) : e(e), s(shift + _Va_shift) {}

    template< typename _I2 >
    Iter(_I2 const &o) : e(o.e), s(o.s) {}

    unsigned char shift() const { return s; }
    unsigned long addr() const { return e->addr() & (~0UL << s); }

    _E *e;
    unsigned char s;
  };

  template
  <
    typename _Entry,
    unsigned _Shift,
    unsigned _Size,
    bool _May_be_leaf,
    bool _Mask = true
  >
  class Traits
  {
  public:
    typedef _Entry Entry;
    typedef typename Entry::Raw Raw;

    enum
    {
      May_be_leaf = _May_be_leaf,
      Shift = _Shift,
      Size = _Size,
      Mask = _Mask
    };
  };

  template< typename _T, unsigned _Shift >
  class Shift
  {
  public:
    typedef _T Orig_list;
    typedef Ptab::Traits
      < typename _T::Entry,
        _T::Shift - _Shift,
        _T::Size,
        _T::May_be_leaf,
        _T::Mask
      > List;
  };

  template< typename _Head, typename _Tail, unsigned _Shift >
  class Shift< List<_Head, _Tail>, _Shift >
  {
  public:
    typedef Ptab::List<_Head, _Tail> Orig_list;
    typedef Ptab::List
    <
      typename Shift<_Head, _Shift>::List,
      typename Shift<_Tail, _Shift>::List
    > List;
  };

  class Address_wrap
  {
  public:
    enum { Shift = 0 };
    typedef Address Value_type;
    static Address val(Address a) { return a; }
  };

  template< typename N, int SHIFT >
  class Page_addr_wrap
  {
  public:
    enum { Shift = SHIFT };
    typedef N Value_type;
    static Address val(N a) { return a.value(); }
  };

  template
  <
    typename _Base_entry,
    typename _Traits,
    typename _Addr = Address_wrap
  >
  class Base
  {
  public:
    typedef typename _Addr::Value_type Va;
    typedef _Traits Traits;
    typedef _Base_entry Entry;
    typedef Ptab::Iter<Entry, _Addr::Shift> Iter;
    typedef typename _Traits::Head L0;

  private:
    typedef Base<_Base_entry, _Traits> This;
    typedef Ptab::Walk<_Traits, Iter> Walk;

  public:
    enum { Depth = Walk::Depth };

    template< typename _Alloc >
    Iter walk(Va virt, unsigned level, _Alloc const &alloc)
    {
      return _base.walk(_Addr::val(virt), level, alloc);
    }

    Iter walk(Va virt, unsigned level = 100) const
    {
      return const_cast<Walk&>(_base).walk(_Addr::val(virt), level, Null_alloc());
    }


    template< typename _Alloc >
    bool sync(Va l_addr, Base< _Base_entry, _Traits, _Addr> const *_r,
              Va r_addr, Va size, unsigned level = 100,
              _Alloc const &alloc = _Alloc())
    {
      Address la = _Addr::val(l_addr);
      Address ra = _Addr::val(r_addr);
      Address sz = _Addr::val(size);
      return _base.sync(la, _r->_base,
                        ra, sz, level, alloc);
    }

    void clear() { _base.clear(); }

    void unmap(Va virt, Va size, unsigned level)
    {
      Address va = _Addr::val(virt);
      unsigned long sz = _Addr::val(size);
      _base.unmap(va, sz, level);
    }

    template< typename _Alloc >
    void map(Address phys, Va virt, Va size, unsigned long attr,
	unsigned level, _Alloc const &alloc = _Alloc())
    {
      Address va = _Addr::val(virt);
      unsigned long sz = _Addr::val(size);
      _base.map(phys, va, sz, attr, level, alloc);
    }

    template< typename _Alloc >
    void destroy(Va start, Va end, unsigned level, _Alloc const &alloc = _Alloc())
    { _base.destroy(_Addr::val(start), _Addr::val(end), level, alloc); }

#if 0
    template< typename _New_alloc >
    Base<_Base_entry, _Traits, _New_alloc, _Addr> *alloc_cast()
    { return reinterpret_cast<Base<_Base_entry, _Traits, _New_alloc, _Addr> *>(this); }

    template< typename _New_alloc >
    Base<_Base_entry, _Traits, _New_alloc, _Addr> const *alloc_cast() const
    { return reinterpret_cast<Base<_Base_entry, _Traits, _New_alloc, _Addr> const *>(this); }
#endif

  private:
    Walk _base;
  };

};
