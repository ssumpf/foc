INTERFACE [ia32 || ux || amd64]:

EXTENSION class Mem_space
{
public:
  typedef Pdir Dir_type;

  /** Return status of v_insert. */
  enum // Status
  {
    Insert_ok = 0,		///< Mapping was added successfully.
    Insert_warn_exists,		///< Mapping already existed
    Insert_warn_attrib_upgrade,	///< Mapping already existed, attribs upgrade
    Insert_err_nomem,		///< Couldn't alloc new page table
    Insert_err_exists		///< A mapping already exists at the target addr
  };

  /** Attribute masks for page mappings. */
  enum Page_attrib
  {
    Page_no_attribs = 0,
    /// Page is writable.
    Page_writable = Pt_entry::Writable,
    Page_cacheable = 0,
    /// Page is noncacheable.
    Page_noncacheable = Pt_entry::Noncacheable | Pt_entry::Write_through,
    /// it's a user page.
    Page_user_accessible = Pt_entry::User,
    /// Page has been referenced
    Page_referenced = Pt_entry::Referenced,
    /// Page is dirty
    Page_dirty = Pt_entry::Dirty,
    Page_references = Page_referenced | Page_dirty,
    /// A mask which contains all mask bits
    Page_all_attribs = Page_writable | Page_noncacheable |
                       Page_user_accessible | Page_referenced | Page_dirty,
  };

  // Mapping utilities

  enum				// Definitions for map_util
  {
    Need_insert_tlb_flush = 0,
    Map_page_size = Config::PAGE_SIZE,
    Page_shift = Config::PAGE_SHIFT,
    Map_superpage_size = Config::SUPERPAGE_SIZE,
    Map_max_address = Mem_layout::User_max,
    Whole_space = MWORD_BITS,
    Identity_map = 0,
  };


  void	page_map	(Address phys, Address virt,
                         Address size, unsigned page_attribs);

  void	page_unmap	(Address virt, Address size);

  void	page_protect	(Address virt, Address size,
                         unsigned page_attribs);

protected:
  // DATA
  Dir_type *_dir;
};

//----------------------------------------------------------------------------
IMPLEMENTATION [ia32 || ux || amd64]:

#include <cstring>
#include <cstdio>
#include "cpu.h"
#include "kdb_ke.h"
#include "l4_types.h"
#include "mem_layout.h"
#include "paging.h"
#include "std_macros.h"




PUBLIC explicit inline
Mem_space::Mem_space(Ram_quota *q) : _quota(q), _dir(0) {}

PROTECTED inline
bool
Mem_space::initialize()
{
  void *b;
  if (EXPECT_FALSE(!(b = Kmem_alloc::allocator()
	  ->q_alloc(_quota, Config::PAGE_SHIFT))))
    return false;

  _dir = static_cast<Dir_type*>(b);
  _dir->clear();	// initialize to zero
  return true; // success
}

PUBLIC
Mem_space::Mem_space(Ram_quota *q, Dir_type* pdir)
  : _quota(q), _dir(pdir)
{
  _kernel_space = this;
  _current.cpu(0) = this;
}


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

PUBLIC inline NEEDS["cpu.h"]
static bool
Mem_space::has_superpages()
{
  return Cpu::have_superpages();
}


PUBLIC static inline NEEDS["mem_unit.h"]
void
Mem_space::tlb_flush(bool = false)
{
  Mem_unit::tlb_flush();
}

PUBLIC static inline
void
Mem_space::tlb_flush_spaces(bool, Mem_space *, Mem_space *)
{
  tlb_flush();
}


IMPLEMENT inline
Mem_space *
Mem_space::current_mem_space(unsigned cpu) /// XXX: do not fix, deprecated, remove!
{
  return _current.cpu(cpu);
}

PUBLIC inline
bool
Mem_space::set_attributes(Addr virt, unsigned page_attribs)
{
  Pdir::Iter i = _dir->walk(virt);

  if (!i.e->valid() || i.shift() != Config::PAGE_SHIFT)
    return 0;

  i.e->del_attr(Page::MAX_ATTRIBS);
  i.e->add_attr(page_attribs);
  return true;
}


PROTECTED inline
void
Mem_space::destroy()
{}

/**
 * Destructor.  Deletes the address space and unregisters it from
 * Space_index.
 */
PRIVATE
void
Mem_space::dir_shutdown()
{
  // free all page tables we have allocated for this address space
  // except the ones in kernel space which are always shared
  _dir->destroy(Virt_addr(0),
                Virt_addr(Kmem::mem_user_max), Pdir::Depth - 1,
                Kmem_alloc::q_allocator(_quota));

}

IMPLEMENT
Mem_space::Status
Mem_space::v_insert(Phys_addr phys, Vaddr virt, Vsize size,
                    unsigned page_attribs, bool upgrade_ignore_size)
{
  // insert page into page table

  // XXX should modify page table using compare-and-swap

  assert_kdb (size == Size(Config::PAGE_SIZE)
              || size == Size(Config::SUPERPAGE_SIZE));
  if (size == Size(Config::SUPERPAGE_SIZE))
    {
      assert (Cpu::have_superpages());
      assert (virt.offset(Size(Config::SUPERPAGE_SIZE)) == 0);
      assert (phys.offset(Size(Config::SUPERPAGE_SIZE)) == 0);
    }

  unsigned level = (size == Size(Config::SUPERPAGE_SIZE) ? (int)Pdir::Super_level : (int)Pdir::Depth);
  unsigned shift = (size == Size(Config::SUPERPAGE_SIZE) ? Config::SUPERPAGE_SHIFT : Config::PAGE_SHIFT);
  unsigned attrs = (size == Size(Config::SUPERPAGE_SIZE) ? (unsigned long)Pt_entry::Pse_bit : 0);

  Pdir::Iter i = _dir->walk(virt, level,
                            Kmem_alloc::q_allocator(_quota));

  if (EXPECT_FALSE(!i.e->valid() && i.shift() != shift))
    return Insert_err_nomem;

  if (EXPECT_FALSE(!upgrade_ignore_size
	&& i.e->valid() && (i.shift() != shift || i.addr() != phys.value())))
    return Insert_err_exists;

  if (i.e->valid())
    {
      if (EXPECT_FALSE((i.e->raw() | page_attribs) == i.e->raw()))
	return Insert_warn_exists;

      i.e->add_attr(page_attribs);
      page_protect (Addr(virt).value(), Size(size).value(), i.e->raw() & Page_all_attribs);

      return Insert_warn_attrib_upgrade;
    }
  else
    {
      *i.e = Addr(phys).value() | Pt_entry::Valid | attrs | page_attribs;
      page_map (Addr(phys).value(), Addr(virt).value(), Size(size).value(), page_attribs);

      return Insert_ok;
    }
}

/**
 * Simple page-table lookup.
 *
 * @param virt Virtual address.  This address does not need to be page-aligned.
 * @return Physical address corresponding to a.
 */
PUBLIC inline NEEDS ["paging.h"]
Address
Mem_space::virt_to_phys(Address virt) const
{
  return dir()->virt_to_phys(virt);
}

/**
 * Simple page-table lookup.
 *
 * @param virt Virtual address.  This address does not need to be page-aligned.
 * @return Physical address corresponding to a.
 */
PUBLIC inline NEEDS ["mem_layout.h"]
Address
Mem_space::pmem_to_phys (Address virt) const
{
  return Mem_layout::pmem_to_phys(virt);
}

/**
 * Simple page-table lookup.
 *
 * This method is similar to Space_context's virt_to_phys().
 * The difference is that this version handles Sigma0's
 * address space with a special case:  For Sigma0, we do not
 * actually consult the page table -- it is meaningless because we
 * create new mappings for Sigma0 transparently; instead, we return the
 * logically-correct result of physical address == virtual address.
 *
 * @param a Virtual address.  This address does not need to be page-aligned.
 * @return Physical address corresponding to a.
 */
PUBLIC inline
virtual Address
Mem_space::virt_to_phys_s0(void *a) const
{
  return dir()->virt_to_phys((Address)a);
}

IMPLEMENT
bool
Mem_space::v_lookup(Vaddr virt, Phys_addr *phys,
                    Size *size, unsigned *page_attribs)
{
  Pdir::Iter i = _dir->walk(virt);
  if (size) *size = Size(1UL << i.shift());

  if (!i.e->valid())
    return false;

  if (phys) *phys = Addr(i.e->addr() & (~0UL << i.shift()));
  if (page_attribs) *page_attribs = (i.e->raw() & Page_all_attribs);

  return true;
}

IMPLEMENT
unsigned long
Mem_space::v_delete(Vaddr virt, Vsize size,
                    unsigned long page_attribs = Page_all_attribs)
{
  unsigned ret;

  // delete pages from page tables
  assert (size == Size(Config::PAGE_SIZE) || size == Size(Config::SUPERPAGE_SIZE));

  if (size == Size(Config::SUPERPAGE_SIZE))
    {
      assert (Cpu::have_superpages());
      assert (!virt.offset(Size(Config::SUPERPAGE_SIZE)));
    }

  Pdir::Iter i = _dir->walk(virt);

  if (EXPECT_FALSE (! i.e->valid()))
    return 0;

  assert (! (i.e->raw() & Pt_entry::global())); // Cannot unmap shared ptables

  ret = i.e->raw() & page_attribs;

  if (! (page_attribs & Page_user_accessible))
    {
      // downgrade PDE (superpage) rights
      i.e->del_attr(page_attribs);
      page_protect (Addr(virt).value(), Size(size).value(), i.e->raw() & Page_all_attribs);
    }
  else
    {
      // delete PDE (superpage)
      *i.e = 0;
      page_unmap (Addr(virt).value(), Size(size).value());
    }

  return ret;
}

/**
 * \brief Free all memory allocated for this Mem_space.
 * \pre Runs after the destructor!
 */
PUBLIC
Mem_space::~Mem_space()
{
  if (_dir)
    {
      dir_shutdown();
      Kmem_alloc::allocator()->q_free(_quota, Config::PAGE_SHIFT, _dir);
    }
}


// --------------------------------------------------------------------
IMPLEMENTATION [ia32 || amd64]:

#include <cassert>
#include "l4_types.h"
#include "kmem.h"
#include "mem_unit.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "logdefs.h"
#include "paging.h"

#include <cstring>
#include "config.h"
#include "kmem.h"

IMPLEMENT inline NEEDS ["cpu.h", "kmem.h"]
void
Mem_space::make_current()
{
  Cpu::set_pdbr((Mem_layout::pmem_to_phys(_dir)));
  _current.cpu(current_cpu()) = this;
}

PUBLIC inline NEEDS ["kmem.h"]
Address
Mem_space::phys_dir()
{
  return Mem_layout::pmem_to_phys(_dir);
}

/*
 * The following functions are all no-ops on native ia32.
 * Pages appear in an address space when the corresponding PTE is made
 * ... unlike Fiasco-UX which needs these special tricks
 */

IMPLEMENT inline
void
Mem_space::page_map (Address, Address, Address, unsigned)
{}

IMPLEMENT inline
void
Mem_space::page_protect (Address, Address, unsigned)
{}

IMPLEMENT inline
void
Mem_space::page_unmap (Address, Address)
{}

IMPLEMENT inline NEEDS["kmem.h", "logdefs.h"]
void
Mem_space::switchin_context(Mem_space *from)
{
  // FIXME: this optimization breaks SMP task deletion, an idle thread
  // may run on an already deleted page table
#if 0
  // never switch to kernel space (context of the idle thread)
  if (dir() == Kmem::dir())
    return;
#endif

  if (from != this)
    {
      CNT_ADDR_SPACE_SWITCH;
      make_current();
    }
}

PROTECTED inline
void
Mem_space::sync_kernel()
{
  _dir->sync(Virt_addr(Mem_layout::User_max), Kmem::dir(),
             Virt_addr(Mem_layout::User_max),
             Virt_addr(-Mem_layout::User_max), Pdir::Super_level,
             Kmem_alloc::q_allocator(_quota));
}

// --------------------------------------------------------------------
IMPLEMENTATION [amd64]:

PUBLIC static inline
Page_number
Mem_space::canonize(Page_number v)
{
  if (v & Virt_addr(1UL << 48))
    v = v | Virt_addr(~0UL << 48);
  return v;
}

// --------------------------------------------------------------------
IMPLEMENTATION [ia32 || ux]:

PUBLIC static inline
Page_number
Mem_space::canonize(Page_number v)
{ return v; }
