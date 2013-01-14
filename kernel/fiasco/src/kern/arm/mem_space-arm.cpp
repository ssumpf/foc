INTERFACE [arm]:

#include "auto_quota.h"
#include "kmem.h"		// for "_unused_*" virtual memory regions
#include "member_offs.h"
#include "paging.h"
#include "types.h"
#include "pagetable.h"
#include "ram_quota.h"

EXTENSION class Mem_space
{
  friend class Jdb;

public:
  typedef Page_table Dir_type;

  /** Return status of v_insert. */
  enum // Status
  {
    Insert_ok = Page_table::E_OK,		///< Mapping was added successfully.
    Insert_err_nomem  = Page_table::E_NOMEM,  ///< Couldn't alloc new page table
    Insert_err_exists = Page_table::E_EXISTS, ///< A mapping already exists at the target addr
    Insert_warn_attrib_upgrade = Page_table::E_UPGRADE,	///< Mapping already existed, attribs upgrade
    Insert_warn_exists,		///< Mapping already existed

  };

  /** Attribute masks for page mappings. */
  enum Page_attrib
  {
    Page_no_attribs = 0,
    /// Page is writable.
    Page_writable = Mem_page_attr::Write,
    Page_user_accessible = Mem_page_attr::User,
    /// Page is noncacheable.
    Page_noncacheable = Page::NONCACHEABLE,
    Page_cacheable = Page::CACHEABLE,
    /// it's a user page (USER_NO | USER_RO = USER_RW).
    /// A mask which contains all mask bits
    Page_all_attribs = Page_user_accessible | Page_writable | Page_cacheable,
    Page_referenced = 0,
    Page_dirty = 0,
    Page_references = 0,
  };

  // Mapping utilities

  enum				// Definitions for map_util
  {
    Need_insert_tlb_flush = 1,
    Map_page_size = Config::PAGE_SIZE,
    Page_shift = Config::PAGE_SHIFT,
    Map_superpage_size = Config::SUPERPAGE_SIZE,
    Map_max_address = Mem_layout::User_max,
    Whole_space = 32,
    Identity_map = 0,
  };


  static void kernel_space(Mem_space *);
  static bool has_superpages() { return true; }


private:
  // DATA
  Dir_type *_dir;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cassert>
#include <cstring>
#include <new>

#include "atomic.h"
#include "bug.h"
#include "config.h"
#include "globals.h"
#include "kdb_ke.h"
#include "l4_types.h"
#include "panic.h"
#include "paging.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "mem_unit.h"


PUBLIC static inline
Mword
Mem_space::xlate_flush(unsigned char rights)
{
  Mword a = Page_references;
  if (rights & L4_fpage::RX)
    a |= Page_all_attribs;
  else if (rights & L4_fpage::W)
    a |= Page_writable;
  return a;
}

PUBLIC static inline
Mword
Mem_space::is_full_flush(unsigned char rights)
{
  return rights & L4_fpage::RX;
}

PUBLIC static inline
unsigned char
Mem_space::xlate_flush_result(Mword attribs)
{
  unsigned char r = 0;
  if (attribs & Page_referenced)
    r |= L4_fpage::RX;

  if (attribs & Page_dirty)
    r |= L4_fpage::W;

  return r;
}

// Mapping utilities

PUBLIC inline NEEDS["mem_unit.h"]
void
Mem_space::tlb_flush(bool force = false)
{
  if (!Have_asids)
    Mem_unit::tlb_flush();
  else if (force && c_asid() != ~0UL)
    Mem_unit::tlb_flush(c_asid());

  // else do nothing, we manage ASID local flushes in v_* already
  // Mem_unit::tlb_flush();
}

PUBLIC static inline NEEDS["mem_unit.h"]
void
Mem_space::tlb_flush_spaces(bool all, Mem_space *s1, Mem_space *s2)
{
  if (all || !Have_asids)
    Mem_unit::tlb_flush();
  else
    {
      if (s1)
	s1->tlb_flush(true);
      if (s2)
	s2->tlb_flush(true);
    }
}


IMPLEMENT inline
Mem_space *Mem_space::current_mem_space(unsigned cpu)
{
  return _current.cpu(cpu);
}


IMPLEMENT inline NEEDS ["kmem.h", Mem_space::c_asid]
void Mem_space::switchin_context(Mem_space *from)
{
#if 0
  // never switch to kernel space (context of the idle thread)
  if (this == kernel_space())
    return;
#endif

  if (from != this)
    make_current();
  else
    tlb_flush(true);
#if 0
  _dir->invalidate((void*)Kmem::ipc_window(0), Config::SUPERPAGE_SIZE * 4,
      c_asid());
#endif

}


IMPLEMENT inline
void Mem_space::kernel_space(Mem_space *_k_space)
{
  _kernel_space = _k_space;
}


inline
static unsigned pd_index(void const *address)
{ return (Mword)address >> 20; /* 1MB steps */ }

inline
static unsigned pt_index(void const *address)
{ return ((Mword)address >> 12) & 255; /* 4KB steps for coarse pts */ }


IMPLEMENT
Mem_space::Status
Mem_space::v_insert(Phys_addr phys, Vaddr virt, Vsize size, unsigned page_attribs,
                    bool upgrade_ignore_size)
{
  Mem_space *c = _current.current();
  bool flush = c == this;

  Pte pte = _dir->walk((void*)virt.value(), size.value(), flush,
                       Kmem_alloc::q_allocator(ram_quota()),
                       c->dir());
  if (pte.valid())
    {
      if (EXPECT_FALSE(!upgrade_ignore_size 
	    && (pte.size() != size.value() || pte.phys() != phys.value())))
	return Insert_err_exists;
      if (pte.attr().get_abstract() == page_attribs)
	return Insert_warn_exists;

      Mem_page_attr a = pte.attr();
      a.set_abstract(a.get_abstract() | page_attribs);
      pte.set(phys.value(), size.value(), a, flush);

      BUG_ON(pte.phys() != phys.value(), "overwrite phys addr: %lx with %lx\n",
             pte.phys(), phys.value());

      if (Have_asids)
        Mem_unit::tlb_flush((void*)virt.value(), c_asid());

      return Insert_warn_attrib_upgrade;
    }
  else if (pte.size() != size.value())
    return Insert_err_nomem;
  else
    {
      // we found an invalid entry for the right size
      Mem_page_attr a(Page::Local_page);
      a.set_abstract(page_attribs);
      pte.set(phys.value(), size.value(), a, flush);
      return Insert_ok;
    }
}


/**
 * Simple page-table lookup.
 *
 * @param virt Virtual address.  This address does not need to be page-aligned.
 * @return Physical address corresponding to a.
 */
PUBLIC inline
Address
Mem_space::virt_to_phys(Address virt) const
{
  Pte pte = _dir->walk((void*)virt, 0, false, Ptab::Null_alloc(), 0);
  if (EXPECT_FALSE(!pte.valid()))
    return ~0UL;

  return (Address)pte.phys((void*)virt);
}

PUBLIC inline NEEDS [Mem_space::virt_to_phys]
Address
Mem_space::pmem_to_phys(Address virt) const
{
  return virt_to_phys(virt);
}

/** Simple page-table lookup.  This method is similar to Mem_space's 
    lookup().  The difference is that this version handles 
    Sigma0's address space with a special case: For Sigma0, we do not 
    actually consult the page table -- it is meaningless because we
    create new mappings for Sigma0 transparently; instead, we return the
    logically-correct result of physical address == virtual address.
    @param a Virtual address.  This address does not need to be page-aligned.
    @return Physical address corresponding to a.
 */
PUBLIC inline
virtual Address
Mem_space::virt_to_phys_s0(void *a) const
{
  return virt_to_phys((Address)a);
}

IMPLEMENT
bool
Mem_space::v_lookup(Vaddr virt, Phys_addr *phys,
                    Size *size, unsigned *page_attribs)
{
  Pte p = _dir->walk( (void*)virt.value(), 0, false, Ptab::Null_alloc(), 0);

  if (size) *size = Size(p.size());
  if (page_attribs) *page_attribs = p.attr().get_abstract();
  // FIXME: we should not use virt but 0 as offset for phys return value!
  if (phys) *phys = Phys_addr(p.phys((void*)virt.value()));
  return p.valid();
}

IMPLEMENT
unsigned long
Mem_space::v_delete(Vaddr virt, Vsize size,
                    unsigned long del_attribs)
{
  (void)size;
  bool flush = _current.current() == this;
  Pte pte = _dir->walk((void*)virt.value(), 0, false, Ptab::Null_alloc(), 0);
  if (EXPECT_FALSE(!pte.valid()))
    return 0;

  BUG_ON(pte.size() != size.value(), "size mismatch: va=%lx sz=%lx dir=%p\n",
         virt.value(), size.value(), _dir);

  Mem_unit::flush_vcache((void*)(virt.value() & ~(pte.size()-1)), 
      (void*)((virt.value() & ~(pte.size()-1)) + pte.size()));

  Mem_page_attr a = pte.attr();
  unsigned long abs_a = a.get_abstract();

  if (!(del_attribs & Page_user_accessible))
    {
      a.set_ap(abs_a & ~del_attribs);
      pte.attr(a, flush);
    }
  else
    pte.set_invalid(0, flush);

  if (Have_asids)
    Mem_unit::tlb_flush((void*)virt.value(), c_asid());

  return abs_a & del_attribs;
}


PUBLIC inline
bool
Mem_space::set_attributes(Address virt, unsigned page_attribs)
{
  Pte p = _dir->walk( (void*)virt, 0, false, Ptab::Null_alloc(), 0);
  if (!p.valid())
  // copy current shared kernel page directory
    return false;

  Mem_page_attr a = p.attr();
  a.set_ap(page_attribs);
  p.attr(a, true);
  return true;
}

/**
 * \brief Free all memory allocated for this Mem_space.
 * \pre Runs after the destructor!
 */
PUBLIC
Mem_space::~Mem_space()
{
  reset_asid();
  if (_dir)
    {
      _dir->free_page_tables(0, (void*)Mem_layout::User_max,
                             Kmem_alloc::q_allocator(ram_quota()));
      Kmem_alloc::allocator()->q_unaligned_free(ram_quota(), sizeof(Page_table), _dir);
    }
}


/** Constructor.  Creates a new address space and registers it with
  * Space_index.
  *
  * Registration may fail (if a task with the given number already
  * exists, or if another thread creates an address space for the same
  * task number concurrently).  In this case, the newly-created
  * address space should be deleted again.
  */
PUBLIC inline
Mem_space::Mem_space(Ram_quota *q)
: _quota(q), _dir(0)
{
  asid(~0UL);
}

PROTECTED inline NEEDS[<new>, "kmem_alloc.h", Mem_space::asid]
bool
Mem_space::initialize()
{
  Auto_quota<Ram_quota> q(ram_quota(), sizeof(Page_table));
  if (EXPECT_FALSE(!q))
    return false;

  _dir = (Page_table*)Kmem_alloc::allocator()->unaligned_alloc(sizeof(Page_table));
  if (!_dir)
    return false;

  new (_dir) Page_table;

  q.release();
  return true;
}

PROTECTED inline
void
Mem_space::sync_kernel()
{
  // copy current shared kernel page directory
  _dir->copy_in((void*)Mem_layout::User_max,
                kernel_space()->_dir,
                (void*)Mem_layout::User_max,
                Mem_layout::Kernel_max - Mem_layout::User_max);
}

PUBLIC
Mem_space::Mem_space(Ram_quota *q, Dir_type* pdir)
  : _quota(q), _dir (pdir)
{
  asid(~0UL);
  _current.cpu(0) = this;
}

PUBLIC static inline
Page_number
Mem_space::canonize(Page_number v)
{ return v; }

//----------------------------------------------------------------------------
IMPLEMENTATION [armv5]:

PRIVATE inline
void
Mem_space::asid(unsigned long)
{}

PRIVATE inline
void
Mem_space::reset_asid()
{}

PUBLIC inline
unsigned long
Mem_space::c_asid() const
{ return 0; }

IMPLEMENT inline
void Mem_space::make_current()
{
  _dir->activate();
  _current.current() = this;
}


//----------------------------------------------------------------------------
INTERFACE [armv6 || armv7]:

EXTENSION class Mem_space
{
public:
  enum { Have_asids = 1 };
private:
  unsigned long _asid[Config::Max_num_cpus];

  static Per_cpu<unsigned char> _next_free_asid;
  static Per_cpu<Mem_space *[256]>   _active_asids;
};

//----------------------------------------------------------------------------
INTERFACE [!(armv6 || armv7)]:

EXTENSION class Mem_space
{
public:
  enum { Have_asids = 0 };
};

//----------------------------------------------------------------------------
IMPLEMENTATION [armv6 || armca8]:

PRIVATE inline static
unsigned long
Mem_space::next_asid(unsigned cpu)
{
  return _next_free_asid.cpu(cpu)++;
}

//----------------------------------------------------------------------------
IMPLEMENTATION [armv7 && armca9]:

PRIVATE inline static
unsigned long
Mem_space::next_asid(unsigned cpu)
{
  if (_next_free_asid.cpu(cpu) == 0)
    ++_next_free_asid.cpu(cpu);
  return _next_free_asid.cpu(cpu)++;
}

//----------------------------------------------------------------------------
IMPLEMENTATION [armv6 || armv7]:

DEFINE_PER_CPU Per_cpu<unsigned char>    Mem_space::_next_free_asid;
DEFINE_PER_CPU Per_cpu<Mem_space *[256]> Mem_space::_active_asids;

PRIVATE inline
void
Mem_space::asid(unsigned long a)
{
  for (unsigned i = 0; i < Config::Max_num_cpus; ++i)
    _asid[i] = a;
}

PUBLIC inline
unsigned long
Mem_space::c_asid() const
{ return _asid[current_cpu()]; }

PRIVATE inline NEEDS[Mem_space::next_asid, "types.h"]
unsigned long
Mem_space::asid()
{
  unsigned cpu = current_cpu();
  if (EXPECT_FALSE(_asid[cpu] == ~0UL))
    {
      // FIFO ASID replacement strategy
      unsigned char new_asid = next_asid(cpu);
      Mem_space **bad_guy = &_active_asids.cpu(cpu)[new_asid];
      while (Mem_space *victim = access_once(bad_guy))
	{
	  // need ASID replacement
	  if (victim == current_mem_space(cpu))
	    {
	      // do not replace the ASID of the current space
	      new_asid = next_asid(cpu);
	      bad_guy = &_active_asids.cpu(cpu)[new_asid];
	      continue;
	    }

	  //LOG_MSG_3VAL(current(), "ASIDr", new_asid, (Mword)*bad_guy, (Mword)this);
	  Mem_unit::tlb_flush(new_asid);

          // If the victim is valid and we get a 1 written to the ASID array
          // then we have to reset the ASID of our victim, else the
          // reset_asid function is currently resetting the ASIDs of the
          // victim on a different CPU.
          if (victim != reinterpret_cast<Mem_space*>(~0UL) &&
              mp_cas(bad_guy, victim, reinterpret_cast<Mem_space*>(1)))
            write_now(&victim->_asid[cpu], ~0UL);
	  break;
	}

      _asid[cpu] = new_asid;
      write_now(bad_guy, this);
    }

  //LOG_MSG_3VAL(current(), "ASID", (Mword)this, _asid[cpu], (Mword)__builtin_return_address(0));
  return _asid[cpu];
};

PRIVATE inline
void
Mem_space::reset_asid()
{
  for (unsigned i = 0; i < Config::Max_num_cpus; ++i)
    {
      unsigned asid = access_once(&_asid[i]);
      if (asid == ~0UL)
        continue;

      Mem_space **a = &_active_asids.cpu(i)[asid];
      if (!mp_cas(a, this, reinterpret_cast<Mem_space*>(~0UL)))
        // It could be our ASID is in the process of being preempted,
        // so wait until this is done.
        while (access_once(a) == reinterpret_cast<Mem_space*>(1))
          ;
    }
}

IMPLEMENT inline NEEDS[Mem_space::asid]
void Mem_space::make_current()
{
  _dir->activate(asid());
  _current.current() = this;
}
