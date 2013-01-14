INTERFACE [io]:

#include "types.h"
#include "mem_space.h"

class Mem_space;
class Space;

/** Wrapper class for io_{map,unmap}.  This class serves as an adapter
    for map<Generic_io_space> to Mem_space.
 */
template< typename SPACE >
class Generic_io_space
{
  friend class Jdb_iomap;

public:
  static char const * const name;

  typedef Page_number Addr;
  typedef Page_count Size;
  typedef Page_number Phys_addr;
  typedef void Reap_list;

  enum
  {
    Need_insert_tlb_flush = 0,
    Need_xcpu_tlb_flush = 0,
    Map_page_size = 1,
    Page_shift = 0,
    Map_superpage_shift = 16,
    Map_superpage_size = 0x10000,
    Map_max_address = 0x10000,
    Whole_space = 16,
    Identity_map = 1,
  };

  // We'd rather like to use a "using Mem_space::Status" declaration here,
  // but that wouldn't make the enum values accessible as
  // Generic_io_space::Insert_ok and so on.
  enum Status
  {
    Insert_ok = 0,		///< Mapping was added successfully.
    Insert_warn_exists,		///< Mapping already existed
    Insert_warn_attrib_upgrade,	///< Mapping already existed, attribs upgrade
    Insert_err_nomem,		///< Couldn't alloc new page table
    Insert_err_exists		///< A mapping already exists at the target addr
  };

  enum Page_attribs
  {
    Page_writable = Mem_space::Page_writable,
    Page_user_accessible = Mem_space::Page_user_accessible,
    Page_references = 0,
    Page_all_attribs = Page_writable | Page_user_accessible
  };

  static Addr map_max_address()
  { return Addr(Map_max_address); }

  static Address superpage_size()
  { return Map_superpage_size; }

  static bool has_superpages()
  { return true; }

  static Phys_addr page_address(Phys_addr o, Size s)
  { return o.trunc(s); }

  static Phys_addr subpage_address(Phys_addr addr, Size offset)
  { return addr | offset; }

private:
  // DATA
  Mword _io_counter;

  Mem_space const *mem_space() const
  { return static_cast<SPACE const *>(this); }

  Mem_space *mem_space()
  { return static_cast<SPACE *>(this); }
};

template< typename SPACE>
char const * const Generic_io_space<SPACE>::name = "Io_space";


//----------------------------------------------------------------------------
IMPLEMENTATION [io]:

#include <cassert>
#include <cstring>

#include "atomic.h"
#include "config.h"
#include "l4_types.h"
#include "kmem_alloc.h"
#include "panic.h"
#include "paging.h"


PUBLIC template< typename SPACE >
static inline
Mword
Generic_io_space<SPACE>::xlate_flush(unsigned char rights)
{
  if (rights)
    return Page_all_attribs;
  else
    return 0;
}

PUBLIC template< typename SPACE >
static inline
Mword
Generic_io_space<SPACE>::is_full_flush(unsigned char rights)
{
  return rights;
}

PUBLIC template< typename SPACE >
static inline
Mword
Generic_io_space<SPACE>::xlate_flush_result(Mword /*attribs*/)
{ return 0; }

PUBLIC template< typename SPACE >
inline
Generic_io_space<SPACE>::Generic_io_space()
  : _io_counter(0)
{}


PUBLIC template< typename SPACE >
Generic_io_space<SPACE>::~Generic_io_space()
{
  if (!mem_space()->dir())
    return;

  Pdir::Iter iopte = mem_space()->dir()->walk(Virt_addr(Mem_layout::Io_bitmap));

  // do we have an IO bitmap?
  if (iopte.e->valid())
    {
      // sanity check
      assert (iopte.shift() != Config::SUPERPAGE_SHIFT);

      Kmem_alloc::allocator()
	->q_free_phys(ram_quota(), Config::PAGE_SHIFT,
	              iopte.e[0].addr());

      if (iopte.e[1].valid())
	Kmem_alloc::allocator()
	  ->q_free_phys(ram_quota(), Config::PAGE_SHIFT,
	                iopte.e[1].addr());

      Pdir::Iter iopde = mem_space()->dir()->walk(Virt_addr(Mem_layout::Io_bitmap), 0);

      // free the page table
      Kmem_alloc::allocator()
	->q_free_phys(ram_quota(), Config::PAGE_SHIFT,
	              iopde.e->addr());

      // free reference
      *iopde.e = 0;
    }
}

PUBLIC template< typename SPACE >
inline
Ram_quota *
Generic_io_space<SPACE>::ram_quota() const
{ return static_cast<SPACE const *>(this)->ram_quota(); }

PRIVATE template< typename SPACE >
inline
bool
Generic_io_space<SPACE>::is_superpage()
{ return _io_counter & 0x10000000; }


//
// Utilities for map<Generic_io_space> and unmap<Generic_io_space>
//

PUBLIC template< typename SPACE >
virtual
bool
Generic_io_space<SPACE>::v_fabricate(Addr address, Phys_addr *phys,
                                     Size *size, unsigned *attribs = 0)
{
  return Generic_io_space::v_lookup(address.trunc(Size(Map_page_size)),
      phys, size, attribs);

}

PUBLIC template< typename SPACE >
inline NEEDS[Generic_io_space::is_superpage]
bool
Generic_io_space<SPACE>::v_lookup(Addr virt, Phys_addr *phys = 0,
                                  Size *size = 0, unsigned *attribs = 0)
{
  if (is_superpage())
    {
      if (size) *size = Size(Map_superpage_size);
      if (phys) *phys = Phys_addr(0);
      if (attribs) *attribs = Page_writable | Page_user_accessible;
      return true;
    }

  if (size) *size = Size(1);

  if (io_lookup(virt.value()))
    {
      if (phys) *phys = virt;
      if (attribs) *attribs = Page_writable | Page_user_accessible;
      return true;
    }

  if (get_io_counter() == 0)
    {
      if (size) *size = Size(Map_superpage_size);
      if (phys) *phys = Phys_addr(0);
    }

  return false;
}

PUBLIC template< typename SPACE >
inline NEEDS [Generic_io_space::is_superpage]
unsigned long
Generic_io_space<SPACE>::v_delete(Addr virt, Size size,
                                  unsigned long page_attribs = Page_all_attribs)
{
  (void)size;
  (void)page_attribs;
  assert (page_attribs == Page_all_attribs);

  if (is_superpage())
    {
      assert (size.value() == Map_superpage_size);

      for (unsigned p = 0; p < Map_max_address; ++p)
	io_delete(p);

      _io_counter = 0;
      return Page_writable | Page_user_accessible;
    }

  assert (size.value() == 1);

  return io_delete(virt.value());
}

PUBLIC template< typename SPACE >
inline
typename Generic_io_space<SPACE>::Status
Generic_io_space<SPACE>::v_insert(Phys_addr phys, Addr virt, Size size,
                                  unsigned page_attribs)
{
  (void)phys;
  (void)size;
  (void)page_attribs;

  assert (phys == virt);
  if (is_superpage() && size.value() == Map_superpage_size)
    return Insert_warn_exists;

  if (get_io_counter() == 0 && size.value() == Map_superpage_size)
    {
      for (unsigned p = 0; p < Map_max_address; ++p)
	io_insert(p);
      _io_counter |= 0x10000000;

      return Insert_ok;
    }

  assert (size.value() == 1);

  return typename Generic_io_space::Status(io_insert(virt.value()));
}


PUBLIC template< typename SPACE >
inline static
void
Generic_io_space<SPACE>::tlb_flush()
{}

PUBLIC template< typename SPACE >
inline static
bool
Generic_io_space<SPACE>::need_tlb_flush()
{ return false; }

//
// IO lookup / insert / delete / counting
//

/** return the IO counter.
 *  @return number of IO ports mapped / 0 if not mapped
 */
PUBLIC template< typename SPACE >
inline NEEDS["paging.h"]
Mword
Generic_io_space<SPACE>::get_io_counter() const
{
  return _io_counter & ~0x10000000;
}


/** Add something the the IO counter.
    @param incr number to add
    @pre 2nd level page table for IO bitmap is present
*/
template< typename SPACE >
inline NEEDS["paging.h"]
void
Generic_io_space<SPACE>::addto_io_counter(int incr)
{
  atomic_add (&_io_counter, incr);
}


/** Lookup one IO port in the IO space.
    @param port_number port address to lookup;
    @return true if mapped
     false if not
 */
PROTECTED template< typename SPACE >
bool
Generic_io_space<SPACE>::io_lookup(Address port_number)
{
  assert(port_number < Mem_layout::Io_port_max);

  // be careful, do not cause page faults here
  // there might be nothing mapped in the IO bitmap

  Address port_addr = get_phys_port_addr(port_number);

  if(port_addr == ~0UL)
    return false;		// no bitmap -> no ports

  // so there is memory mapped in the IO bitmap
  char *port = static_cast<char *>(Kmem::phys_to_virt(port_addr));

  // bit == 1 disables the port
  // bit == 0 enables the port
  return !(*port & get_port_bit(port_number));
}


/** Enable one IO port in the IO space.
    This function is called in the context of the IPC sender!
    @param port_number address of the port
    @return Insert_warn_exists if some ports were mapped in that IO page
       Insert_err_nomem if memory allocation failed
       Insert_ok if otherwise insertion succeeded
 */
PROTECTED template< typename SPACE >
typename Generic_io_space<SPACE>::Status
Generic_io_space<SPACE>::io_insert(Address port_number)
{
  assert(port_number < Mem_layout::Io_port_max);

  Address port_virt = Mem_layout::Io_bitmap + (port_number >> 3);
  Address port_phys = mem_space()->virt_to_phys(port_virt);

  if (port_phys == ~0UL)
    {
      // nothing mapped! Get a page and map it in the IO bitmap
      void *page;
      if (!(page = Kmem_alloc::allocator()->q_alloc(ram_quota(),
                                                    Config::PAGE_SHIFT)))
	return Insert_err_nomem;

      // clear all IO ports
      // bit == 1 disables the port
      // bit == 0 enables the port
      memset(page, 0xff, Config::PAGE_SIZE);

      Mem_space::Status status =
	mem_space()->v_insert(
	    Mem_space::Phys_addr(Mem_layout::pmem_to_phys(page)),
	    Mem_space::Addr(port_virt & Config::PAGE_MASK),
	    Mem_space::Size(Config::PAGE_SIZE), Page_writable);

      if (status == Mem_space::Insert_err_nomem)
	{
	  Kmem_alloc::allocator()->free(Config::PAGE_SHIFT, page);
	  ram_quota()->free(Config::PAGE_SIZE);
	  return Insert_err_nomem;
	}

      // we've been careful, so insertion should have succeeded
      assert(status == Mem_space::Insert_ok);

      port_phys = mem_space()->virt_to_phys(port_virt);
      assert(port_phys != ~0UL);
    }

  // so there is memory mapped in the IO bitmap -- write the bits now
  Unsigned8 *port = static_cast<Unsigned8 *> (Kmem::phys_to_virt(port_phys));

  if (*port & get_port_bit(port_number)) // port disabled?
    {
      *port &= ~get_port_bit(port_number);
      addto_io_counter(1);
      return Insert_ok;
    }

  // already enabled
  return Insert_warn_exists;
}


/** Disable one IO port in the IO space.
    @param port_number port to disable
 */
PROTECTED template< typename SPACE >
unsigned
Generic_io_space<SPACE>::io_delete(Address port_number)
{
  assert(port_number < Mem_layout::Io_port_max);

  // be careful, do not cause page faults here
  // there might be nothing mapped in the IO bitmap

  Address port_addr = get_phys_port_addr(port_number);

  if (port_addr == ~0UL)
    // nothing mapped -> nothing to delete
    return 0;

  // so there is memory mapped in the IO bitmap -> disable the ports
  char *port = static_cast<char *>(Kmem::phys_to_virt(port_addr));

  // bit == 1 disables the port
  // bit == 0 enables the port
  if(!(*port & get_port_bit(port_number)))    // port enabled ??
    {
      *port |= get_port_bit(port_number);
      addto_io_counter(-1);

      return Page_writable | Page_user_accessible;
    }

  return 0;
}

template< typename SPACE >
INLINE NEEDS["config.h"]
Address
Generic_io_space<SPACE>::get_phys_port_addr(Address const port_number) const
{
  return mem_space()->virt_to_phys(Mem_layout::Io_bitmap + (port_number >> 3));
}

template< typename SPACE >
INLINE
Unsigned8
Generic_io_space<SPACE>::get_port_bit(Address const port_number) const
{
  return 1 << (port_number & 7);
}


PUBLIC template< typename SPACE >
inline static
Page_number
Generic_io_space<SPACE>::canonize(Page_number v)
{ return v; }
