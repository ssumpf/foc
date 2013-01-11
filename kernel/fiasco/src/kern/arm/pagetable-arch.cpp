//---------------------------------------------------------------------------
INTERFACE[arm]:

class Mem_page_attr
{
  friend class Pte;

public:
  unsigned long get_ap() const;
  void set_ap(unsigned long ap);

private:
  unsigned long _a;
};

//---------------------------------------------------------------------------
INTERFACE[arm && armv5]:

EXTENSION class Mem_page_attr
{
public:
  enum
  {
    Write   = 0x400,
    User    = 0x800,
    Ap_mask = 0xc00,
  };
};

//---------------------------------------------------------------------------
INTERFACE[arm && (armv6 || armv7)]:

EXTENSION class Mem_page_attr
{
public:
  enum
  {
    Write   = 0x200,
    User    = 0x020,
    Ap_mask = 0x220,
  };
};

//---------------------------------------------------------------------------
INTERFACE[arm && !(mpcore || armca9)]:

EXTENSION class Mem_page_attr
{
public:
  // do not use Shaed bit on non MP CPUs because this leads to uncached memory
  // accesses most of the time!
  enum { Shared  = 0 };
};

EXTENSION class Page_table
{
public:
  enum { Ttbr_bits = 0x0 };
};

//---------------------------------------------------------------------------
INTERFACE[arm && (mpcore || armca9)]:

EXTENSION class Mem_page_attr
{
public:
  // use shared bit on MP CPUs as we need cache coherency
  enum { Shared  = 0x400 };
};

//---------------------------------------------------------------------------
INTERFACE[arm && mpcore]:

EXTENSION class Page_table
{
public:
  enum { Ttbr_bits = 0xa };
};

//---------------------------------------------------------------------------
INTERFACE[arm && armca9]:

EXTENSION class Page_table
{
public:
  enum
  {
    Ttbr_bits =    (1 << 1)            // S, Shareable bit
                |  (1 << 3)            // RGN, Region bits, Outer WriteBackWriteAlloc
                |  (0 << 0) | (1 << 6) // IRGN, Inner region bits, WB-WA
                |  (1 << 5)            // NOS
                ,
  };
};

//---------------------------------------------------------------------------
INTERFACE [arm]:

#include "paging.h"
#include "per_cpu_data.h"

class Ram_quota;

class Pte
{
public:
//private:
  unsigned long _pt;
  Mword *_pte;

public:
  Pte(Page_table *pt, unsigned level, Mword *pte)
  : _pt((unsigned long)pt | level), _pte(pte)
  {}
};


EXTENSION class Page_table
{
private:
  Mword raw[4096];
  enum
  {
    Pt_base_mask     = 0xfffffc00,
    Pde_type_coarse  = 0x01,
  };
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && vcache]:

PUBLIC static inline
bool
Pte::need_cache_clean()
{
  return false;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && !vcache && !armca9]:

PUBLIC static inline
bool
Pte::need_cache_clean()
{
  return true;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && !vcache && armca9]:

PUBLIC static inline
bool
Pte::need_cache_clean()
{
  return false;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cassert>
#include <cstring>

#include "mem_unit.h"
#include "kdb_ke.h"
#include "ram_quota.h"

PUBLIC inline explicit
Mem_page_attr::Mem_page_attr(unsigned long attr) : _a(attr)
{}

PRIVATE inline
unsigned long
Mem_page_attr::raw() const
{ return _a; }

PUBLIC inline
void
Mem_page_attr::set_caching(unsigned long del, unsigned long set)
{
  del &= Page::Cache_mask;
  set &= Page::Cache_mask;
  _a = (_a & ~del) | set;
}

PUBLIC inline NEEDS[Mem_page_attr::get_ap]
unsigned long
Mem_page_attr::get_abstract() const
{ return get_ap() | (_a & Page::Cache_mask); }

PUBLIC inline NEEDS[Mem_page_attr::set_ap]
void
Mem_page_attr::set_abstract(unsigned long a)
{
  _a = (_a & ~Page::Cache_mask) | (a & Page::Cache_mask);
  set_ap(a);
}

PUBLIC inline NEEDS[Mem_page_attr::get_ap]
bool
Mem_page_attr::permits(unsigned long attr)
{ return (get_ap() & attr) == attr; }

PUBLIC inline
unsigned long
Pte::valid() const
{ return *_pte & 3; }

PUBLIC inline
unsigned long
Pte::phys() const
{
  switch(_pt & 3)
    {
    case 0:
      switch (*_pte & 3)
	{
	case 2:  return *_pte & ~((1 << 20) - 1); // 1MB
	default: return ~0UL;
	}
    case 1:
      switch (*_pte & 3)
	{
	case 2: return *_pte & ~((4 << 10) - 1);
	default: return ~0UL;
	}
    default: return ~0UL;
    }
}

PUBLIC inline
unsigned long
Pte::phys(void *virt)
{
  unsigned long p = phys();
  return p | (((unsigned long)virt) & (size()-1));
}

PUBLIC inline
unsigned long
Pte::lvl() const
{ return (_pt & 3); }

PUBLIC inline
unsigned long
Pte::raw() const
{ return *_pte; }

PUBLIC inline
bool
Pte::superpage() const
{ return !(_pt & 3) && ((*_pte & 3) == 2); }

PUBLIC inline
unsigned long
Pte::size() const
{
  switch(_pt & 3)
    {
    case 0:
      switch (*_pte & 3)
	{
	case 2:  return 1 << 20; // 1MB
	default: return 1 << 20;
	}
    case 1:
      switch (*_pte & 3)
	{
	case 1: return 64 << 10;
	case 2: return 4 << 10;
	case 3: return 1 << 10;
	default: return 4 << 10;
	}
    default: return 0;
    }
}


PRIVATE inline NEEDS["mem_unit.h"]
void
Pte::__set(unsigned long v, bool write_back)
{
  *_pte = v;
  if (write_back || need_cache_clean())
    Mem_unit::clean_dcache(_pte);
}

PUBLIC inline NEEDS[Pte::__set]
void
Pte::set_invalid(unsigned long val, bool write_back)
{ __set(val & ~3, write_back); }


//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && armv5]:

IMPLEMENT inline
unsigned long
Mem_page_attr::get_ap() const
{
  static unsigned char const _map[4] = { 0x8, 0x4, 0x0, 0xc };
  return ((unsigned long)_map[(_a >> 10) & 0x3]) << 8UL;
}

IMPLEMENT inline
void
Mem_page_attr::set_ap(unsigned long ap)
{
  static unsigned char const _map[4] = { 0x4, 0x4, 0x0, 0xc };
  _a = (_a & ~0xc00) | (((unsigned long)_map[(ap >> 10) & 0x3]) << 8UL);
}

PUBLIC inline NEEDS[Pte::__set, Mem_page_attr::raw]
void
Pte::set(Address phys, unsigned long size, Mem_page_attr const &attr,
    bool write_back)
{
  switch (_pt & 3)
    {
    case 0:
      if (size != (1 << 20))
	return;
      __set(phys | (attr.raw() & Page::MAX_ATTRIBS) | 2, write_back);
      break;
    case 1:
	{
	  if (size != (4 << 10))
	    return;
	  unsigned long ap = attr.raw() & 0xc00; ap |= ap >> 2; ap |= ap >> 4;
	  __set(phys | (attr.raw() & 0x0c) | ap | 2, write_back);
	}
      break;
    }
}

PUBLIC inline NEEDS[Mem_page_attr::Mem_page_attr]
Mem_page_attr
Pte::attr() const { return Mem_page_attr(*_pte & 0xc0c); }

PUBLIC inline NEEDS["mem_unit.h"]
void
Pte::attr(Mem_page_attr const &attr, bool write_back)
{
  switch (_pt & 3)
    {
    case 0:
      __set((*_pte & ~0xc0c) | (attr.raw() & 0xc0c), write_back);
      break;
    case 1:
	{
	  unsigned long ap = attr.raw() & 0xc00; ap |= ap >> 2; ap |= ap >> 4;
	  __set((*_pte & ~0xffc) | (attr.raw() & 0x0c) | ap, write_back);
	}
      break;
    }
}

PUBLIC /*inline*/
void Page_table::activate()
{
  Pte p = walk(this, 0, false, Ptab::Null_alloc(), 0);
  Mem_unit::flush_vcache();
  asm volatile (
      "mcr p15, 0, r0, c8, c7, 0 \n" // TLB flush
      "mcr p15, 0, %0, c2, c0    \n" // pdbr

      "mrc p15, 0, r1, c2, c0    \n"
      "mov r1, r1                \n"
      "sub pc, pc, #4            \n"
      :
      : "r" (p.phys(this))
      : "r1");
}


//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && (armv6 || armv7)]:

IMPLEMENT inline
unsigned long
Mem_page_attr::get_ap() const
{
  return (_a & User) | ((_a & Write) ^ Write);
}

IMPLEMENT inline NEEDS[Mem_page_attr::raw]
void
Mem_page_attr::set_ap(unsigned long ap)
{
  _a = (_a & ~(User | Write)) | (ap & User) | ((ap & Write) ^ Write) | 0x10;
}

PUBLIC inline NEEDS[Pte::__set]
void
Pte::set(Address phys, unsigned long size, Mem_page_attr const &attr,
         bool write_back)
{
  switch (_pt & 3)
    {
    case 0:
      if (size != (1 << 20))
	return;
      {
	unsigned long a = attr.raw() & 0x0c; // C & B
	a |= ((attr.raw() & 0xff0) | Mem_page_attr::Shared) << 6;
	__set(phys | a | 0x2, write_back);
      }
      break;
    case 1:
      if (size != (4 << 10))
	return;
      __set(phys | (attr.raw() & Page::MAX_ATTRIBS) | 0x2 | Mem_page_attr::Shared, write_back);
      break;
    }
}

PUBLIC inline NEEDS[Mem_page_attr::raw]
Mem_page_attr
Pte::attr() const
{
  switch (_pt & 3)
    {
    case 0:
	{
	  unsigned long a = *_pte & 0x0c; // C & B
	  a |= (*_pte >> 6) & 0xff0;
	  return Mem_page_attr(a);
	}
    case 1:
    default:
      return Mem_page_attr(*_pte & Page::MAX_ATTRIBS);
    }
}

PUBLIC inline NEEDS["mem_unit.h", Mem_page_attr::raw]
void
Pte::attr(Mem_page_attr const &attr, bool write_back)
{
  switch (_pt & 3)
    {
    case 1:
      __set((*_pte & ~Page::MAX_ATTRIBS)
            | (attr.raw() & Page::MAX_ATTRIBS), write_back);
      break;
    case 0:
	{
	  unsigned long a = attr.raw() & 0x0c;
	  a |= (attr.raw() & 0xff0) << 6;
	  __set((*_pte & ~0x3fcc) | a, write_back);
	}
      break;
    }
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [armv6 || armca8]:

PUBLIC
void Page_table::activate(unsigned long asid)
{
  Pte p = walk(this, 0, false, Ptab::Null_alloc(), 0);
  asm volatile (
      "mcr p15, 0, %2, c7, c5, 6    \n" // bt flush
      "mcr p15, 0, r0, c7, c10, 4   \n" // dsb
      "mcr p15, 0, %0, c2, c0       \n" // set TTBR
      "mcr p15, 0, r0, c7, c10, 4   \n" // dsb
      "mcr p15, 0, %1, c13, c0, 1   \n" // set new ASID value
      "mcr p15, 0, r0, c7, c5, 4    \n" // isb
      "mcr p15, 0, %2, c7, c5, 6    \n" // bt flush
      "mrc p15, 0, r1, c2, c0       \n"
      "mov r1, r1                   \n"
      "sub pc, pc, #4               \n"
      :
      : "r" (p.phys(this) | Ttbr_bits), "r"(asid), "r" (0)
      : "r1");
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [armv7 && armca9]:

PUBLIC
void Page_table::activate(unsigned long asid)
{
  Pte p = walk(this, 0, false, Ptab::Null_alloc(), 0);
  asm volatile (
      "mcr p15, 0, %2, c7, c5, 6    \n" // bt flush
      "dsb                          \n"
      "mcr p15, 0, %2, c13, c0, 1   \n" // change ASID to 0
      "isb                          \n"
      "mcr p15, 0, %0, c2, c0       \n" // set TTBR
      "isb                          \n"
      "mcr p15, 0, %1, c13, c0, 1   \n" // set new ASID value
      "isb                          \n"
      "mcr p15, 0, %2, c7, c5, 6    \n" // bt flush
      "isb                          \n"
      "mov r1, r1                   \n"
      "sub pc, pc, #4               \n"
      :
      : "r" (p.phys(this) | Ttbr_bits), "r"(asid), "r" (0)
      : "r1");
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && !mp]:

PRIVATE inline
Mword
Page_table::current_virt_to_phys(void *virt)
{
  return walk(virt, 0, false, Ptab::Null_alloc(), 0).phys(virt);
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && mp]:

/*
 * This version for MP avoids calling Page_table::current() which calls
 * current_cpu() which is not available on the boot-stack. That's why we use
 * the following way of doing a virt->phys translation on the current page
 * table.
 */
PRIVATE inline
Mword
Page_table::current_virt_to_phys(void *virt)
{
  Mword phys;
  Mword offset = (Mword)virt & ~Config::PAGE_MASK;
  asm volatile("mcr p15,0,%1,c7,c8,0 \n"
               "mrc p15,0,%0,c7,c4,0 \n"
               : "=r" (phys)
	       : "r" ((Mword)virt & Config::PAGE_MASK));
  return (phys & Config::PAGE_MASK) | offset;
}


//-----------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cassert>
#include "bug.h"
#include "auto_quota.h"
#include "mem.h"
#if 0
IMPLEMENT
void *Page_table::operator new(size_t s) throw()
{
  (void)s;
  assert(s == 16*1024);
  return alloc()->alloc(14); // 2^14 = 16K
}

IMPLEMENT
void Page_table::operator delete(void *b)
{
  alloc()->free(14, b);
}
#endif

IMPLEMENT
Page_table::Page_table()
{
  Mem::memset_mwords(raw, 0, sizeof(raw) / sizeof(Mword));
  if (Pte::need_cache_clean())
    Mem_unit::clean_dcache(raw, (char *)raw + sizeof(raw));
}


PUBLIC template< typename ALLOC >
void Page_table::free_page_tables(void *start, void *end, ALLOC const &a)
{
  for (unsigned i = (Address)start >> 20; i < ((Address)end >> 20); ++i)
    {
      Pte p(this, 0, raw + i);
      if (p.valid() && !p.superpage())
	{
	  void *pt = (void*)Mem_layout::phys_to_pmem(p.raw() & Pt_base_mask);

          BUG_ON(pt == (void*)~0UL, "cannot get virtual (pmem) address for %lx (pte @ %p)\n",
              p.raw() & Pt_base_mask, p._pte);

	  a.free(pt, 1<<10);
	}
    }
}

PRIVATE inline
static unsigned Page_table::pd_index( void const *const address )
{
  return (Mword)address >> 20; // 1MB steps
}

PRIVATE inline
static unsigned Page_table::pt_index( void const *const address )
{
  return ((Mword)address >> 12) & 255; // 4KB steps for coarse pts
}

PUBLIC template< typename Alloc >
inline NEEDS[<cassert>, "bug.h", Page_table::pd_index,
             Page_table::current_virt_to_phys, Page_table::pt_index]
Pte
Page_table::walk(void *va, unsigned long size, bool write_back, Alloc const &q,
                 Page_table *ldir)
{
  unsigned const pd_idx = pd_index(va);

  Mword *pt = 0;

  Pte pde(this, 0, raw + pd_idx);

  if (!pde.valid())
    {
      if (size == (4 << 10))
	{
	  assert (q.valid());
          pt = (Mword*)q.alloc(1<<10);
          if (EXPECT_FALSE(!pt))
            return pde;

	  Mem::memset_mwords(pt, 0, 1024 >> 2);

	  if (write_back || Pte::need_cache_clean())
	    Mem_unit::clean_dcache(pt, (char*)pt + 1024);

	  raw[pd_idx] = ldir->current_virt_to_phys(pt) | Pde_type_coarse;

	  if (write_back || Pte::need_cache_clean())
	    Mem_unit::clean_dcache(raw + pd_idx);
	}
      else
	return pde;
    }
  else if (pde.superpage())
    return pde;

  if (!pt)
    pt = (Mword *)Mem_layout::phys_to_pmem(pde.raw() & Pt_base_mask);

  BUG_ON(pt == (void*)~0UL, "could not get virtual address for %lx (from pte @%p)\n",
         pde.raw(), pde._pte);

  return Pte(this, 1, pt + pt_index(va));
}


IMPLEMENT
void Page_table::init()
{
  unsigned domains      = 0x0001;

  asm volatile (
      "mcr p15, 0, %0, c3, c0       \n" // domains
      :
      : "r"(domains) );
}


IMPLEMENT /*inline*/
void Page_table::copy_in(void *my_base, Page_table *o,
			 void *base, size_t size, unsigned long asid)
{
  unsigned pd_idx = pd_index(my_base);
  unsigned pd_idx_max = pd_index(my_base) + pd_index((void*)size);
  unsigned o_pd_idx = pd_index(base);
  bool need_flush = false;

  //printf("copy_in: %03x-%03x from %03x\n", pd_idx, pd_idx_max, o_pd_idx);

  if (asid != ~0UL)
    {
      for (unsigned i = pd_idx;  i < pd_idx_max; ++i)
	if (Pte(this, 0, raw + i).valid())
	  {
	    Mem_unit::flush_vdcache();
	    need_flush = true;
	    break;
	  }
    }

  for (unsigned i = pd_idx; i < pd_idx_max; ++i, ++o_pd_idx)
    raw[i] = o->raw[o_pd_idx];

  if (Pte::need_cache_clean())
    Mem_unit::clean_dcache(raw + pd_idx, raw + pd_idx_max);

  if (need_flush && (asid != ~0UL))
    Mem_unit::dtlb_flush(asid);
}

#if 0
PUBLIC
void
Page_table::invalidate(void *my_base, unsigned size, unsigned long asid = ~0UL)
{
  unsigned pd_idx = pd_index(my_base);
  unsigned pd_idx_max = pd_index(my_base) + pd_index((void*)size);
  bool need_flush = false;

  //printf("invalidate: %03x-%03x\n", pd_idx, pd_idx_max);

  if (asid != ~0UL)
    {
      for (unsigned i = pd_idx; i < pd_idx_max; ++i)
	if (Pte(this, 0, raw + i).valid())
	  {
	    Mem_unit::flush_vdcache();
	    need_flush = true;
	    break;
	  }
    }

  for (unsigned i = pd_idx; i < pd_idx_max; ++i)
    raw[i] = 0;

  // clean the caches if manipulating the current pt or in the case if phys.
  // tagged caches.
  if ((asid != ~0UL) || Pte::need_cache_clean())
    Mem_unit::clean_dcache(raw + pd_idx, raw + pd_idx_max);

  if (need_flush && (asid != ~0UL))
    Mem_unit::tlb_flush(asid);
}
#endif

IMPLEMENT
void *
Page_table::dir() const
{
  return const_cast<Page_table *>(this);
}

