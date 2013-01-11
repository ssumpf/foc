INTERFACE:

#include "assert_opt.h"
#include "l4_types.h"
#include "space.h"

class Mapdb;

namespace Mu {
template<typename SPACE>
struct Virt_addr { typedef Page_number Type; };

template<>
struct Virt_addr<Obj_space> { typedef Obj_space::Addr Type; };


template< typename SPACE, typename M >
inline
Mword v_delete(M &m, Mword flush_rights, bool full_flush)
{
  SPACE* child_space = m->space();
  assert_opt (child_space);
  Mword res = child_space->v_delete(m.page(), m.size(), flush_rights);
  (void) full_flush;
  assert_kdb (full_flush != child_space->v_lookup(m.page()));
  return res;
}


}

template< typename SPACE >
class Map_traits
{
public:
  typedef Page_number Addr;
  typedef Page_count Size;

  static Addr get_addr(L4_fpage const &fp);
  static void constraint(Addr &snd_addr, Size &snd_size,
                         Addr &rcv_addr, Size const &rcv_size,
                         Addr const &hot_spot);
  static bool match(L4_fpage const &from, L4_fpage const &to);
  static bool free_object(typename SPACE::Phys_addr o,
                          typename SPACE::Reap_list **reap_list);
};


class Reap_list
{
private:
  Kobject *_h;
  Kobject **_t;

public:
  Reap_list() : _h(0), _t(&_h) {}
  Kobject ***list() { return &_t; }
};

namespace Mu {
template<>
inline
Mword v_delete<Obj_space>(Kobject_mapdb::Iterator &m, Mword flush_rights, bool /*full_flush*/)
{
  Obj_space::Entry *c = static_cast<Obj_space::Entry*>(*m);

  if (c->valid())
    {
      if (flush_rights & L4_fpage::R)
        c->invalidate();
      else
        c->del_rights(flush_rights & L4_fpage::WX);
    }
  return 0;
}
}

//------------------------------------------------------------------------
IMPLEMENTATION:

#include <cassert>

#include "config.h"
#include "context.h"
#include "kobject.h"
#include "paging.h"
#include "warn.h"


IMPLEMENT template<typename SPACE>
inline
bool
Map_traits<SPACE>::match(L4_fpage const &, L4_fpage const &)
{ return false; }

IMPLEMENT template<typename SPACE>
inline
bool
Map_traits<SPACE>::free_object(typename SPACE::Phys_addr,
                               typename SPACE::Reap_list **)
{ return false; }


PUBLIC template< typename SPACE >
static inline
void
Map_traits<SPACE>::attribs(L4_msg_item /*control*/, L4_fpage const &/*fp*/,
                           unsigned long *del_attr, unsigned long *set_attr)
{ *del_attr = 0; *set_attr = 0; }

PUBLIC template< typename SPACE >
static inline
unsigned long
Map_traits<SPACE>::apply_attribs(unsigned long attribs,
                                 typename SPACE::Phys_addr &,
                                 unsigned long set_attr, unsigned long del_attr)
{ return (attribs & ~del_attr) | set_attr; }

PRIVATE template<typename SPACE>
static inline
void
Map_traits<SPACE>::identity_constraint(Addr &snd_addr, Size &snd_size,
                                       Addr rcv_addr, Size rcv_size)
{
  if (rcv_addr > snd_addr)
    {
      if (rcv_addr - snd_addr < snd_size)
	snd_size -= rcv_addr - snd_addr;
      else
	snd_size = Size(0);
      snd_addr = rcv_addr;
    }

  if (snd_size > rcv_size)
    snd_size = rcv_size;
}

PRIVATE template<typename SPACE>
static inline
void
Map_traits<SPACE>::free_constraint(Addr &snd_addr, Size &snd_size,
                                   Addr &rcv_addr, Size rcv_size,
                                   Addr const &hot_spot)
{
  if (rcv_size >= snd_size)
    rcv_addr += hot_spot.offset(rcv_size).trunc(snd_size);
  else
    {
      snd_addr += hot_spot.offset(snd_size).trunc(rcv_size);
      snd_size = rcv_size;
      // reduce size of address range
    }
}

IMPLEMENT template<typename SPACE>
inline
void
Map_traits<SPACE>::constraint(Addr &snd_addr, Size &snd_size,
                              Addr &rcv_addr, Size const &rcv_size,
                              Addr const &hot_spot)
{
  if (SPACE::Identity_map)
    identity_constraint(snd_addr, snd_size, rcv_addr, rcv_size);
  else
    free_constraint(snd_addr, snd_size, rcv_addr, rcv_size, hot_spot);
}



//-------------------------------------------------------------------------
IMPLEMENTATION [io]:

IMPLEMENT template<>
inline
bool
Map_traits<Io_space>::match(L4_fpage const &from, L4_fpage const &to)
{ return from.is_iopage() && (to.is_iopage() || to.is_all_spaces()); }

IMPLEMENT template<>
inline
Map_traits<Io_space>::Addr
Map_traits<Io_space>::get_addr(L4_fpage const &fp)
{ return Addr(fp.io_address()); }



//-------------------------------------------------------------------------
IMPLEMENTATION:

IMPLEMENT template<>
inline
bool
Map_traits<Mem_space>::match(L4_fpage const &from, L4_fpage const &to)
{ 
  return from.is_mempage() && (to.is_all_spaces() || to.is_mempage());
}

IMPLEMENT template<>
inline
Map_traits<Mem_space>::Addr
Map_traits<Mem_space>::get_addr(L4_fpage const &fp)
{ return Addr(fp.mem_address()); }

IMPLEMENT template<>
inline
void
Map_traits<Mem_space>::attribs(L4_msg_item control, L4_fpage const &fp,
    unsigned long *del_attr, unsigned long *set_attr)
{
  *del_attr = (fp.rights() & L4_fpage::W) ? 0 : Mem_space::Page_writable;
  short cache = control.attr() & 0x70;

  if (cache & L4_msg_item::Caching_opt)
    {
      *del_attr |= Page::Cache_mask;

      if (cache == L4_msg_item::Cached)
	*set_attr = Page::CACHEABLE;
      else if (cache == L4_msg_item::Buffered)
	*set_attr = Page::BUFFERED;
      else
	*set_attr = Page::NONCACHEABLE;
    }
  else
    *set_attr = 0;
}


IMPLEMENT template<>
inline
bool
Map_traits<Obj_space>::match(L4_fpage const &from, L4_fpage const &to)
{ return from.is_objpage() && (to.is_objpage() || to.is_all_spaces()); }


IMPLEMENT template<>
inline
Map_traits<Obj_space>::Addr
Map_traits<Obj_space>::get_addr(L4_fpage const &fp)
{ return Addr(fp.obj_index()); }

IMPLEMENT template<>
inline
bool
Map_traits<Obj_space>::free_object(Obj_space::Phys_addr o,
                                   Obj_space::Reap_list **reap_list)
{
  if (o->map_root()->no_mappings())
    {
      o->initiate_deletion(reap_list);
      return true;
    }

  return false;
}

IMPLEMENT template<>
inline
void
Map_traits<Obj_space>::attribs(L4_msg_item control, L4_fpage const &fp,
    unsigned long *del_attr, unsigned long *set_attr)
{
  *set_attr = 0;
  *del_attr = (~(fp.rights() | (L4_msg_item::C_weak_ref ^ control.attr())));
}

IMPLEMENT template<>
static inline
unsigned long
Map_traits<Obj_space>::apply_attribs(unsigned long attribs,
                                     Obj_space::Phys_addr &a,
                                     unsigned long set_attr,
                                     unsigned long del_attr)
{
  if (attribs & del_attr & L4_msg_item::C_obj_specific_rights)
    a = a->downgrade(del_attr);

  return (attribs & ~del_attr) | set_attr;
}


/** Flexpage mapping.
    divert to mem_map (for memory fpages) or io_map (for IO fpages)
    @param from source address space
    @param fp_from flexpage descriptor for virtual-address space range
	in source address space
    @param to destination address space
    @param fp_to flexpage descriptor for virtual-address space range
	in destination address space
    @param offs sender-specified offset into destination flexpage
    @param grant if set, grant the fpage, otherwise map
    @pre page_aligned(offs)
    @return IPC error
    L4_fpage from_fp, to_fp;
    Mword control;code that describes the status of the operation
*/
// Don't inline -- it eats too much stack.
// inline NEEDS ["config.h", io_map]
L4_error
fpage_map(Space *from, L4_fpage fp_from, Space *to,
          L4_fpage fp_to, L4_msg_item control, Reap_list *r)
{
 if (Map_traits<Mem_space>::match(fp_from, fp_to))
    return mem_map(from, fp_from, to, fp_to, control);

#ifdef CONFIG_IO_PROT
  if (Map_traits<Io_space>::match(fp_from, fp_to))
    return io_map(from, fp_from, to, fp_to, control);
#endif

  if (Map_traits<Obj_space>::match(fp_from, fp_to))
    return obj_map(from, fp_from, to, fp_to, control, r->list());

  return L4_error::None;
}

/** Flexpage unmapping.
    divert to mem_fpage_unmap (for memory fpages) or
    io_fpage_unmap (for IO fpages)
    @param space address space that should be flushed
    @param fp    flexpage descriptor of address-space range that should
                 be flushed
    @param me_too If false, only flush recursive mappings.  If true,
                 additionally flush the region in the given address space.
    @param flush_mode determines which access privileges to remove.
    @return combined (bit-ORed) access status of unmapped physical pages
*/
// Don't inline -- it eats too much stack.
// inline NEEDS ["config.h", io_fpage_unmap]
unsigned
fpage_unmap(Space *space, L4_fpage fp, L4_map_mask mask, Kobject ***rl)
{
  unsigned ret = 0;

  if (fp.is_iopage() || fp.is_all_spaces())
    ret |= io_fpage_unmap(space, fp, mask);

  if (fp.is_objpage() || fp.is_all_spaces())
    ret |= obj_fpage_unmap(space, fp, mask, rl);

  if (fp.is_mempage() || fp.is_all_spaces())
    ret |= mem_fpage_unmap(space, fp, mask);

  return ret;
}

PUBLIC
void
Reap_list::del()
{
  if (EXPECT_TRUE(!_h))
    return;

  for (Kobject *reap = _h; reap; reap = reap->_next_to_reap)
    reap->destroy(list());

  current()->rcu_wait();

  for (Kobject *reap = _h; reap;)
    {
      Kobject *d = reap;
      reap = reap->_next_to_reap;
      if (d->put())
	delete d;
    }

  _h = 0;
  _t = &_h;
}

PUBLIC inline
Reap_list::~Reap_list()
{ del(); }

//////////////////////////////////////////////////////////////////////
//
// Utility functions for all address-space types
//

#include "mapdb.h"

inline
template <typename SPACE, typename MAPDB>
L4_error
map(MAPDB* mapdb,
    SPACE* from, Space *from_id,
    Page_number _snd_addr,
    Page_count snd_size,
    SPACE* to, Space *to_id,
    Page_number _rcv_addr,
    bool grant, unsigned attrib_add, unsigned attrib_del,
    typename SPACE::Reap_list **reap_list = 0)
{
  enum
  {
    PAGE_SIZE = SPACE::Map_page_size,
    PAGE_MASK = ~(PAGE_SIZE - 1),
    SUPERPAGE_SIZE = SPACE::Map_superpage_size,
    SUPERPAGE_MASK = ~((SUPERPAGE_SIZE - 1) >> SPACE::Page_shift)
  };

  typedef typename SPACE::Size Size;
  typedef typename MAPDB::Mapping Mapping;
  typedef typename MAPDB::Frame Frame;
  typedef typename Mu::Virt_addr<SPACE>::Type Vaddr;
  typedef Map_traits<SPACE> Mt;

  L4_error condition = L4_error::None;

  // FIXME: make this debugging code optional
  bool no_page_mapped = true;

  Vaddr rcv_addr(_rcv_addr);
  Vaddr snd_addr(_snd_addr);
  const Vaddr rcv_start = rcv_addr;
  const Page_count rcv_size = snd_size;
  // We now loop through all the pages we want to send from the
  // sender's address space, looking up appropriate parent mappings in
  // the mapping data base, and entering a child mapping and a page
  // table entry for the receiver.

  // Special care is taken for 4MB page table entries we find in the
  // sender's address space: If what we will create in the receiver is
  // not a 4MB-mapping, too, we have to find the correct parent
  // mapping for the new mapping database entry: This is the sigma0
  // mapping for all addresses != the 4MB page base address.

  // When overmapping an existing page, flush the interfering
  // physical page in the receiver, even if it is larger than the
  // mapped page.

  // verify sender and receiver virtual addresses are still within
  // bounds; if not, bail out.  Sigma0 may send from any address (even
  // from an out-of-bound one)
  Page_count size;
  bool need_tlb_flush = false;
  bool need_xcpu_tlb_flush = false;
  Page_number const to_max = to->map_max_address();
  Page_number const from_max = from->map_max_address();
  Size const from_superpage_size(from->superpage_size());
  bool const has_superpages = to->has_superpages();

  for (;
       snd_size                               // pages left for sending?
       && rcv_addr < to_max
       && snd_addr < from_max;

       rcv_addr += size,
       snd_addr += size,
       snd_size -= size)
    {
      // Reset the increment size to one page.
      size = Size(PAGE_SIZE);

      // First, look up the page table entries in the sender and
      // receiver address spaces.

      // Sender lookup.
      // make gcc happy, initialized later anyway
      typename SPACE::Phys_addr s_phys;
      Size s_size(0);
      unsigned s_attribs = 0;

      // Sigma0 special case: Sigma0 doesn't need to have a
      // fully-constructed page table, and it can fabricate mappings
      // for all physical addresses.:435
      if (EXPECT_FALSE(! from->v_fabricate(snd_addr, &s_phys,
                                           &s_size, &s_attribs)))
	continue;

      // We have a mapping in the sender's address space.
      // FIXME: make this debugging code optional
      no_page_mapped = false;

      // Receiver lookup.

      // The may be used uninitialized warning for this variable is bogus
      // the v_lookup function must initialize the value if it returns true.
      typename SPACE::Phys_addr r_phys;
      Size r_size;
      unsigned r_attribs;

      // Compute attributes for to-be-inserted frame
      typename SPACE::Phys_addr i_phys = s_phys;
      Size i_size = s_size;
      bool const rcv_page_mapped
	= to->v_lookup(rcv_addr, &r_phys, &r_size, &r_attribs);
      // See if we have to degrade to non-superpage mappings
      if (has_superpages && i_size == from_superpage_size)
	{
	  if (i_size > snd_size
	      // want to send less that a superpage?
	      || i_size > r_size         // not enough space for superpage map?
	      || snd_addr.offset(Size(SUPERPAGE_SIZE)) // snd page not aligned?
	      || rcv_addr.offset(Size(SUPERPAGE_SIZE)) // rcv page not aligned?
	      || (rcv_addr + from_superpage_size > rcv_start + rcv_size))
						      // rcv area to small?
	    {
	      // We map a 4K mapping from a 4MB mapping
	      i_size = Size(PAGE_SIZE);

	      if (Size super_offset = snd_addr.offset(Size(SUPERPAGE_SIZE)))
		{
		  // Just use OR here because i_phys may already contain
		  // the offset. (As is on ARM)
		  i_phys = SPACE::subpage_address(i_phys, super_offset);
		}

	      if (grant)
		{
		  WARN("XXX Can't GRANT page from superpage (%p: " L4_PTR_FMT
		       " -> %p: " L4_PTR_FMT "), demoting to MAP\n",
		       from_id, snd_addr.value(), to_id, rcv_addr.value());
		  grant = 0;
		}
	    }
	}

      // Also, look up mapping database entry.  Depending on whether
      // we can overmap, either look up the destination mapping first
      // (and compute the sender mapping from it) or look up the
      // sender mapping directly.
      Mapping* sender_mapping = 0;
      // mapdb_frame will be initialized by the mapdb lookup function when
      // it returns true, so don't care about "may be use uninitialized..."
      Frame mapdb_frame;
      bool doing_upgrade = false;

      if (rcv_page_mapped)
	{
	  // We have something mapped.

	  // Check if we can upgrade mapping.  Otherwise, flush target
	  // mapping.
	  if (! grant                         // Grant currently always flushes
	      && r_size <= i_size             // Rcv frame in snd frame
	      && SPACE::page_address(r_phys, i_size) == i_phys
	      && (sender_mapping = mapdb->check_for_upgrade(r_phys, from_id, snd_addr, to_id, rcv_addr, &mapdb_frame)))
	    doing_upgrade = true;

	  if (! sender_mapping)	// Need flush
	    {
	      unmap(mapdb, to, to_id, rcv_addr.trunc(r_size), r_size,
		    L4_fpage::RWX, L4_map_mask::full(), reap_list);
	    }
	}

      if (! sender_mapping && mapdb->valid_address(s_phys))
	{
	  if (EXPECT_FALSE(! mapdb->lookup(from_id,
					   snd_addr.trunc(s_size), s_phys,
					   &sender_mapping, &mapdb_frame)))
	    continue;		// someone deleted this mapping in the meantime
	}

      // from here mapdb_frame is always initialized, so ignore the warning
      // in grant / insert

      // At this point, we have a lookup for the sender frame (s_phys,
      // s_size, s_attribs), the max. size of the receiver frame
      // (r_phys), the sender_mapping, and whether a receiver mapping
      // already exists (doing_upgrade).

      unsigned i_attribs
        = Mt::apply_attribs(s_attribs, i_phys, attrib_add, attrib_del);

      // Loop increment is size of insertion
      size = i_size;

      // Do the actual insertion.
      typename SPACE::Status status
	= to->v_insert(i_phys, rcv_addr, i_size, i_attribs);

      switch (status)
	{
	case SPACE::Insert_warn_exists:
	case SPACE::Insert_warn_attrib_upgrade:
	case SPACE::Insert_ok:

	  assert_kdb (mapdb->valid_address(s_phys) || status == SPACE::Insert_ok);
          // Never doing upgrades for mapdb-unmanaged memory

	  if (grant)
	    {
	      if (mapdb->valid_address(s_phys))
		if (EXPECT_FALSE(!mapdb->grant(mapdb_frame, sender_mapping,
			                       to_id, rcv_addr)))
		  {
		    // Error -- remove mapping again.
		    to->v_delete(rcv_addr, i_size);

		    // may fail due to quota limits
		    condition = L4_error::Map_failed;
		    break;
		  }

	      from->v_delete(snd_addr.trunc(s_size), s_size);
	      need_tlb_flush = true;
	    }
	  else if (status == SPACE::Insert_ok)
	    {
	      assert_kdb (!doing_upgrade);

	      if (mapdb->valid_address(s_phys)
		  && !mapdb->insert(mapdb_frame, sender_mapping,
				    to_id, rcv_addr,
				    i_phys, i_size))
		{
		  // Error -- remove mapping again.
		  to->v_delete(rcv_addr, i_size);

		  // XXX This is not race-free as the mapping could have
		  // been used in the mean-time, but we do not care.
		  condition = L4_error::Map_failed;
		  break;
		}
	    }

	  if (SPACE::Need_xcpu_tlb_flush && SPACE::Need_insert_tlb_flush)
	    need_xcpu_tlb_flush = true;

	  break;

	case SPACE::Insert_err_nomem:
	  condition = L4_error::Map_failed;
	  break;

	case SPACE::Insert_err_exists:
	  WARN("map (%s) skipping area (%p/%lx): " L4_PTR_FMT
	       " -> %p/%lx: " L4_PTR_FMT "(%lx)", SPACE::name,
	       from_id, Kobject_dbg::pointer_to_id(from_id), snd_addr.value(),
	       to_id, Kobject_dbg::pointer_to_id(to_id), rcv_addr.value(), i_size.value());
	  // Do not flag an error here -- because according to L4
	  // semantics, it isn't.
	  break;
	}

      if (sender_mapping)
	mapdb->free(mapdb_frame);

      if (!condition.ok())
	break;
    }

  if (need_tlb_flush)
    from->tlb_flush();

  if (SPACE::Need_xcpu_tlb_flush && need_xcpu_tlb_flush)
    Context::xcpu_tlb_flush(false, to, from);

  // FIXME: make this debugging code optional
  if (EXPECT_FALSE(no_page_mapped))
    {
      WARN("nothing mapped: (%s) from [%p/%lx]: " L4_PTR_FMT
           " size: " L4_PTR_FMT " to [%p/%lx]\n", SPACE::name,
           from_id, Kobject_dbg::pointer_to_id(from_id), snd_addr.value(), rcv_size.value(),
           to_id, Kobject_dbg::pointer_to_id(to_id));
    }

  return condition;
}


template <typename SPACE, typename MAPDB>
unsigned
unmap(MAPDB* mapdb, SPACE* space, Space *space_id,
      Page_number start, Page_count size, unsigned char rights,
      L4_map_mask mask, typename SPACE::Reap_list **reap_list)
{

  typedef typename SPACE::Size Size;
  typedef typename SPACE::Addr Addr;
  typedef typename MAPDB::Mapping Mapping;
  typedef typename MAPDB::Iterator Iterator;
  typedef typename MAPDB::Frame Frame;
  typedef typename Mu::Virt_addr<SPACE>::Type Vaddr;

  bool me_too = mask.self_unmap();

  Mword flushed_rights = 0;
  Page_number end = start + size;
  Page_number const map_max = space->map_max_address();

  // make gcc happy, initialized later anyway
  typename SPACE::Phys_addr phys;
  Page_count phys_size;
  Vaddr page_address;

  Mword const flush_rights = SPACE::xlate_flush(rights);
  bool const full_flush = SPACE::is_full_flush(rights);
  bool need_tlb_flush = false;
  bool need_xcpu_tlb_flush = false;

  // iterate over all pages in "space"'s page table that are mapped
  // into the specified region
  for (Vaddr address(start);
       address < end && address < map_max;
       address = page_address + phys_size)
    {
      // for amd64-mem_space's this will skip the hole in the address space
      address = SPACE::canonize(address);

      bool have_page;

	{
	  Size ps;
	  have_page = space->v_fabricate(address, &phys, &ps);
	  phys_size = ps;
	}

      page_address = address.trunc(phys_size);

      // phys_size and page_address have now been set up, allowing the
      // use of continue (which evaluates the for-loop's iteration
      // expression involving these to variables).

      if (! have_page)
	continue;

      if (me_too)
	{
	  assert_kdb (address == page_address
		  || phys_size == Size(SPACE::Map_superpage_size));

	  // Rewind flush address to page address.  We always flush
	  // the whole page, even if it is larger than the specified
	  // flush area.
	  address = page_address;
	  if (end < address + phys_size)
	    end = address + phys_size;
	}

      // all pages shall be handled by our mapping data base
      assert_kdb (mapdb->valid_address(phys));

      Mapping *mapping;
      Frame mapdb_frame;

      if (! mapdb->lookup(space_id, page_address, phys,
			  &mapping, &mapdb_frame))
	// someone else unmapped faster
	continue;		// skip

      Mword page_rights = 0;

      // Delete from this address space
      if (me_too)
	{
	  page_rights |=
	    space->v_delete(address, phys_size, flush_rights);

          // assert_kdb (full_flush != space->v_lookup(address));
	  need_tlb_flush = true;
	  need_xcpu_tlb_flush = true;
	}

      // now delete from the other address spaces
      for (Iterator m(mapdb_frame, mapping, address, end);
	   m;
	   ++m)
	{
	  page_rights |= Mu::v_delete<SPACE>(m, flush_rights, full_flush);
	  need_xcpu_tlb_flush = true;
	}

      flushed_rights |= page_rights;

      // Store access attributes for later retrieval
      save_access_attribs(mapdb, mapdb_frame, mapping,
			  space, page_rights, page_address, phys, phys_size,
			  me_too);

      if (full_flush)
	mapdb->flush(mapdb_frame, mapping, mask, address, end);

      if (full_flush)
	Map_traits<SPACE>::free_object(phys, reap_list);

      mapdb->free(mapdb_frame);
    }

  if (need_tlb_flush)
    space->tlb_flush();

  if (SPACE::Need_xcpu_tlb_flush && need_xcpu_tlb_flush)
    Context::xcpu_tlb_flush(true, space, 0);

  return SPACE::xlate_flush_result(flushed_rights);
}

//----------------------------------------------------------------------------
IMPLEMENTATION[!io || ux]:

// Empty dummy functions when I/O protection is disabled
inline
void init_mapdb_io(Space *)
{}

inline
L4_error
io_map(Space *, L4_fpage const &, Space *, L4_fpage const &, L4_msg_item)
{
  return L4_error::None;
}

inline
unsigned
io_fpage_unmap(Space * /*space*/, L4_fpage const &/*fp*/, L4_map_mask)
{
  return 0;
}

