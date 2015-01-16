INTERFACE [arm]:

#include "auto_quota.h"
#include "kmem.h"		// for "_unused_*" virtual memory regions
#include "member_offs.h"
#include "paging.h"
#include "types.h"
#include "ram_quota.h"

EXTENSION class Mem_space
{
  friend class Jdb;

public:
  typedef Pdir Dir_type;

  /** Return status of v_insert. */
  enum // Status
  {
    Insert_ok = 0,		///< Mapping was added successfully.
    Insert_err_exists, ///< A mapping already exists at the target addr
    Insert_warn_attrib_upgrade,	///< Mapping already existed, attribs upgrade
    Insert_err_nomem,  ///< Couldn't alloc new page table
    Insert_warn_exists,		///< Mapping already existed

  };

  // Mapping utilities
  enum				// Definitions for map_util
  {
    Need_insert_tlb_flush = 1,
    Map_page_size = Config::PAGE_SIZE,
    Page_shift = Config::PAGE_SHIFT,
    Whole_space = 32,
    Identity_map = 0,
  };


  static void kernel_space(Mem_space *);

private:
  // DATA
  Dir_type *_dir;
  Phys_mem_addr _dir_phys;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cassert>
#include <cstring>
#include <new>

#include "atomic.h"
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
bool
Mem_space::is_full_flush(L4_fpage::Rights rights)
{
  return rights & L4_fpage::Rights::R();
}

// Mapping utilities

PUBLIC inline NEEDS["mem_unit.h"]
void
Mem_space::tlb_flush(bool force = false)
{
  if (!Have_asids)
    Mem_unit::tlb_flush();
  else if (force && c_asid() != Mem_unit::Asid_invalid)
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
Mem_space *Mem_space::current_mem_space(Cpu_number cpu)
{
  return _current.cpu(cpu);
}

PUBLIC inline
bool
Mem_space::set_attributes(Virt_addr virt, Attr page_attribs,
                          bool writeback, Mword asid)
{
   auto i = _dir->walk(virt);

  if (!i.is_valid())
    return false;

  i.set_attribs(page_attribs);
  i.write_back_if(writeback, asid);
  return true;
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
}


IMPLEMENT inline
void Mem_space::kernel_space(Mem_space *_k_space)
{
  _kernel_space = _k_space;
}

IMPLEMENT
Mem_space::Status
Mem_space::v_insert(Phys_addr phys, Vaddr virt, Page_order size,
                    Attr page_attribs)
{
  bool const flush = _current.current() == this;
  assert (cxx::get_lsb(Phys_addr(phys), size) == 0);
  assert (cxx::get_lsb(Virt_addr(virt), size) == 0);

  int level;
  for (level = 0; level <= Pdir::Depth; ++level)
    if (Page_order(Pdir::page_order_for_level(level)) <= size)
      break;

  auto i = _dir->walk(virt, level, Pte_ptr::need_cache_write_back(flush),
                      Kmem_alloc::q_allocator(_quota));

  if (EXPECT_FALSE(!i.is_valid() && i.level != level))
    return Insert_err_nomem;

  if (EXPECT_FALSE(i.is_valid()
                   && (i.level != level || Phys_addr(i.page_addr()) != phys)))
    return Insert_err_exists;

  if (i.is_valid())
    {
      if (EXPECT_FALSE(!i.add_attribs(page_attribs)))
        return Insert_warn_exists;

      i.write_back_if(flush, c_asid());
      return Insert_warn_attrib_upgrade;
    }
  else
    {
      i.create_page(phys, page_attribs);
      i.write_back_if(flush, Mem_unit::Asid_invalid);

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
  return dir()->virt_to_phys(virt);
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
                    Page_order *order, Attr *page_attribs)
{
  auto i = _dir->walk(virt);
  if (order) *order = Page_order(i.page_order());

  if (!i.is_valid())
    return false;

  if (phys) *phys = Virt_addr(i.page_addr());
  if (page_attribs) *page_attribs = i.attribs();

  return true;
}

IMPLEMENT
L4_fpage::Rights
Mem_space::v_delete(Vaddr virt, Page_order size,
                    L4_fpage::Rights page_attribs)
{
  (void) size;
  assert (cxx::get_lsb(Virt_addr(virt), size) == 0);
  auto i = _dir->walk(virt);

  if (EXPECT_FALSE (! i.is_valid()))
    return L4_fpage::Rights(0);

  L4_fpage::Rights ret = i.access_flags();

  if (! (page_attribs & L4_fpage::Rights::R()))
    i.del_rights(page_attribs);
  else
    i.clear();

  i.write_back_if(_current.current() == this, c_asid());

  return ret;
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

      // free all page tables we have allocated for this address space
      // except the ones in kernel space which are always shared
      _dir->destroy(Virt_addr(0UL),
                    Virt_addr(Mem_layout::User_max), 0, Pdir::Depth,
                    Kmem_alloc::q_allocator(_quota));
      // free all unshared page table levels for the kernel space
      if (Virt_addr(Mem_layout::User_max) < Virt_addr(~0UL))
        _dir->destroy(Virt_addr(Mem_layout::User_max + 1),
                      Virt_addr(~0UL), 0, Pdir::Super_level,
                      Kmem_alloc::q_allocator(_quota));
      Kmem_alloc::allocator()->q_unaligned_free(ram_quota(), sizeof(Dir_type), _dir);
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
  asid(Mem_unit::Asid_invalid);
}

PROTECTED inline NEEDS[<new>, "kmem_alloc.h", Mem_space::asid]
bool
Mem_space::initialize()
{
  Auto_quota<Ram_quota> q(ram_quota(), sizeof(Dir_type));
  if (EXPECT_FALSE(!q))
    return false;

  _dir = (Dir_type*)Kmem_alloc::allocator()->unaligned_alloc(sizeof(Dir_type));
  if (!_dir)
    return false;

  _dir->clear(Pte_ptr::need_cache_write_back(false));
  _dir_phys = Phys_mem_addr(Kmem_space::kdir()->virt_to_phys((Address)_dir));

  q.release();
  return true;
}

PUBLIC
Mem_space::Mem_space(Ram_quota *q, Dir_type* pdir)
  : _quota(q), _dir (pdir)
{
  asid(Mem_unit::Asid_invalid);
  _current.cpu(Cpu_number::boot_cpu()) = this;
  _dir_phys = Phys_mem_addr(Kmem_space::kdir()->virt_to_phys((Address)_dir));
}

PUBLIC static inline
Page_number
Mem_space::canonize(Page_number v)
{ return v; }

IMPLEMENTATION [arm && !arm_lpae]:

PUBLIC static
void
Mem_space::init_page_sizes()
{
  add_page_size(Page_order(Config::PAGE_SHIFT));
  add_page_size(Page_order(20)); // 1MB
}

//----------------------------------------------------------------------------
IMPLEMENTATION [arm && !hyp]:

PROTECTED inline
int
Mem_space::sync_kernel()
{
  return _dir->sync(Virt_addr(Mem_layout::User_max + 1), kernel_space()->_dir,
                    Virt_addr(Mem_layout::User_max + 1),
                    Virt_size(-(Mem_layout::User_max + 1)), Pdir::Super_level,
                    Pte_ptr::need_cache_write_back(this == _current.current()),
                    Kmem_alloc::q_allocator(_quota));
}

PUBLIC inline NEEDS [Mem_space::virt_to_phys]
Address
Mem_space::pmem_to_phys(Address virt) const
{
  return virt_to_phys(virt);
}

//----------------------------------------------------------------------------
IMPLEMENTATION [arm && hyp]:

static Address __mem_space_syscall_page;

IMPLEMENT inline
template< typename T >
T
Mem_space::peek_user(T const *addr)
{
  Address pa = virt_to_phys((Address)addr);
  if (pa == ~0UL)
    return ~0;

  Address ka = Mem_layout::phys_to_pmem(pa);
  if (ka == ~0UL)
    return ~0;

  return *reinterpret_cast<T const *>(ka);
}

PROTECTED static
void
Mem_space::set_syscall_page(void *p)
{
  __mem_space_syscall_page = (Address)p;
}

PROTECTED
int
Mem_space::sync_kernel()
{
  auto pte = _dir->walk(Virt_addr(Mem_layout::Kern_lib_base),
      Pdir::Depth, true, Kmem_alloc::q_allocator(ram_quota()));
  if (pte.level < Pdir::Depth - 1)
    return -1;

  extern char kern_lib_start[];

  Phys_mem_addr pa((Address)kern_lib_start - Mem_layout::Map_base
                   + Mem_layout::Sdram_phys_base);
  pte.create_page(pa, Page::Attr(Page::Rights::URX(), Page::Type::Normal(),
                                 Page::Kern::Global()));

  pte.write_back_if(true, Mem_unit::Asid_kernel);

  pte = _dir->walk(Virt_addr(Mem_layout::Syscalls),
      Pdir::Depth, true, Kmem_alloc::q_allocator(ram_quota()));

  if (pte.level < Pdir::Depth - 1)
    return -1;

  pa = Phys_mem_addr(__mem_space_syscall_page);
  pte.create_page(pa, Page::Attr(Page::Rights::URX(), Page::Type::Normal(),
                                 Page::Kern::Global()));

  pte.write_back_if(true, Mem_unit::Asid_kernel);

  return 0;
}

PUBLIC inline NEEDS [Mem_space::virt_to_phys]
Address
Mem_space::pmem_to_phys(Address virt) const
{
  return Kmem_space::kdir()->virt_to_phys(virt);
}

//----------------------------------------------------------------------------
IMPLEMENTATION [armv5 || armv6 || armv7]:

IMPLEMENT inline
void
Mem_space::v_set_access_flags(Vaddr, L4_fpage::Rights)
{}

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
  _current.current() = this;
  Mem_unit::flush_vcache();
  asm volatile (
      "mcr p15, 0, r0, c8, c7, 0 \n" // TLBIALL
      "mcr p15, 0, %0, c2, c0    \n" // TTBR0

      "mrc p15, 0, r1, c2, c0    \n"
      "mov r1, r1                \n"
      "sub pc, pc, #4            \n"
      :
      : "r" (cxx::int_value<Phys_mem_addr>(_dir_phys))
      : "r1");
}


//----------------------------------------------------------------------------
INTERFACE [armv6 || armv7]:

EXTENSION class Mem_space
{
public:
  enum { Have_asids = 1 };
private:
  typedef Per_cpu_array<unsigned long> Asid_array;
  Asid_array _asid;

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
Mem_space::next_asid(Cpu_number cpu)
{
  return _next_free_asid.cpu(cpu)++;
}

//----------------------------------------------------------------------------
IMPLEMENTATION [armv7 && armca9]:

PRIVATE inline static
unsigned long
Mem_space::next_asid(Cpu_number cpu)
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
  for (Asid_array::iterator i = _asid.begin(); i != _asid.end(); ++i)
    *i = a;
}

PUBLIC inline
unsigned long
Mem_space::c_asid() const
{ return _asid[current_cpu()]; }

PRIVATE inline NEEDS[Mem_space::next_asid, "types.h"]
unsigned long
Mem_space::asid()
{
  Cpu_number cpu = current_cpu();
  if (EXPECT_FALSE(_asid[cpu] == Mem_unit::Asid_invalid))
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

          // If the victim is valid and we get a 1 written to the ASID array
          // then we have to reset the ASID of our victim, else the
          // reset_asid function is currently resetting the ASIDs of the
          // victim on a different CPU.
          if (victim != reinterpret_cast<Mem_space*>(~0UL) &&
              mp_cas(bad_guy, victim, reinterpret_cast<Mem_space*>(1)))
            write_now(&victim->_asid[cpu], (Mword)Mem_unit::Asid_invalid);
	  break;
	}

      _asid[cpu] = new_asid;
      Mem_unit::tlb_flush(new_asid);
      write_now(bad_guy, this);
    }

  //LOG_MSG_3VAL(current(), "ASID", (Mword)this, _asid[cpu], (Mword)__builtin_return_address(0));
  return _asid[cpu];
};

PRIVATE inline
void
Mem_space::reset_asid()
{
  for (Cpu_number i = Cpu_number::first(); i < Config::max_num_cpus(); ++i)
    {
      unsigned long asid = access_once(&_asid[i]);
      if (asid == Mem_unit::Asid_invalid)
        continue;

      Mem_space **a = &_active_asids.cpu(i)[asid];
      if (!mp_cas(a, this, reinterpret_cast<Mem_space*>(~0UL)))
        // It could be our ASID is in the process of being preempted,
        // so wait until this is done.
        while (access_once(a) == reinterpret_cast<Mem_space*>(1))
          ;
    }
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [armv6 || armca8]:

IMPLEMENT inline
void Mem_space::make_current()
{
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
      : "r" (Phys_mem_addr::val(_dir_phys) | Page::Ttbr_bits), "r"(asid()), "r" (0)
      : "r1");
  _current.current() = this;
}


//-----------------------------------------------------------------------------
IMPLEMENTATION [armv7 && armca9 && !arm_lpae]:

IMPLEMENT inline
void
Mem_space::make_current()
{
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
      : "r" (Phys_mem_addr::val(_dir_phys) | Page::Ttbr_bits), "r"(asid()), "r" (0)
      : "r1");
  _current.current() = this;
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [armv7 && arm_lpae]:

PUBLIC static
void
Mem_space::init_page_sizes()
{
  add_page_size(Page_order(Config::PAGE_SHIFT));
  add_page_size(Page_order(21)); // 2MB
  add_page_size(Page_order(30)); // 1GB
}

//----------------------------------------------------------------------------
IMPLEMENTATION [armv7 && arm_lpae && !hyp]:

IMPLEMENT inline
void
Mem_space::make_current()
{
  asm volatile (
      "mcr p15, 0, %2, c7, c5, 6    \n" // bt flush
      "isb                          \n"
      "mcrr p15, 0, %0, %1, c2      \n" // set TTBR
      "isb                          \n"
      "mcr p15, 0, %2, c7, c5, 6    \n" // bt flush
      "isb                          \n"
      "mov r1, r1                   \n"
      "sub pc, pc, #4               \n"
      :
      : "r" (Phys_mem_addr::val(_dir_phys)), "r"(asid() << 16), "r" (0)
      : "r1");
  _current.current() = this;
}


//----------------------------------------------------------------------------
IMPLEMENTATION [armv7 && arm_lpae && hyp]:

IMPLEMENT inline
void
Mem_space::make_current()
{

  asm volatile (
      "mcr p15, 0, %2, c7, c5, 6    \n" // bt flush
      "isb                          \n"
      "mcrr p15, 6, %0, %1, c2      \n" // set TTBR
      "isb                          \n"
      "mcr p15, 0, %2, c7, c5, 6    \n" // bt flush
      "isb                          \n"
      "mov r1, r1                   \n"
      "sub pc, pc, #4               \n"
      :
      : "r" (Phys_mem_addr::val(_dir_phys)), "r"(asid() << 16), "r" (0)
      : "r1");

  _current.current() = this;
}
