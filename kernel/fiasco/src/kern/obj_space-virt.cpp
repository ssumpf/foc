INTERFACE:

#include "mem.h"
#include "mem_space.h"
#include "ram_quota.h"

EXTENSION class Generic_obj_space
{
  // do not use the virtually mapped cap table in
  // v_lookup and v_insert, because the map logic needs the kernel
  // address for link pointers in the map-nodes and these addresses must
  // be valid in all address spaces.
  enum { Optimize_local = 0 };
};

IMPLEMENTATION:

#include <cstring>
#include <cassert>

#include "atomic.h"
#include "config.h"
#include "cpu.h"
#include "kdb_ke.h"
#include "kmem_alloc.h"
#include "mem_layout.h"

PRIVATE template< typename SPACE > inline
Mem_space *
Generic_obj_space<SPACE>::mem_space()
{ return static_cast<SPACE*>(this); }

PRIVATE  template< typename SPACE >
static inline NEEDS["mem_layout.h"]
typename Generic_obj_space<SPACE>::Entry *
Generic_obj_space<SPACE>::cap_virt(Address index)
{ return reinterpret_cast<Entry*>(Mem_layout::Caps_start) + index; }

PRIVATE  template< typename SPACE >
inline NEEDS["mem_space.h", "mem_layout.h", Generic_obj_space::cap_virt]
typename Generic_obj_space<SPACE>::Entry *
Generic_obj_space<SPACE>::alien_lookup(Address index)
{
  Mem_space *ms = mem_space();

  Address phys = Address(ms->virt_to_phys((Address)cap_virt(index)));
  if (EXPECT_FALSE(phys == ~0UL))
    return 0;

  return reinterpret_cast<Entry*>(Mem_layout::phys_to_pmem(phys));
}

PRIVATE template< typename SPACE >
typename Generic_obj_space<SPACE>::Entry *
Generic_obj_space<SPACE>::get_cap(Address index)
{ return alien_lookup(index); }

PUBLIC  template< typename SPACE >
inline NEEDS["mem_space.h"]
Ram_quota *
Generic_obj_space<SPACE>::ram_quota() const
{ return static_cast<SPACE const *>(this)->ram_quota(); }


PRIVATE  template< typename SPACE >
/*inline NEEDS["kmem_alloc.h", <cstring>, "ram_quota.h",
                     Generic_obj_space::cap_virt]*/
typename Generic_obj_space<SPACE>::Entry *
Generic_obj_space<SPACE>::caps_alloc(Address virt)
{
  Address cv = (Address)cap_virt(virt);
  void *mem = Kmem_alloc::allocator()->q_unaligned_alloc(ram_quota(), Config::PAGE_SIZE);

  if (!mem)
    return 0;

  add_dbg_info(mem, this, virt);

  Mem::memset_mwords(mem, 0, Config::PAGE_SIZE / sizeof(Mword));

  Mem_space::Status s;
  s = mem_space()->v_insert(
      Mem_space::Phys_addr::create(Mem_space::kernel_space()->virt_to_phys((Address)mem)),
      Mem_space::Addr::create(cv).trunc(Mem_space::Size::create(Config::PAGE_SIZE)),
      Mem_space::Size::create(Config::PAGE_SIZE),
      Mem_space::Page_cacheable | Mem_space::Page_writable
      | Mem_space::Page_referenced | Mem_space::Page_dirty);

  switch (s)
    {
    case Insert_ok:
      break;
    case Insert_warn_exists:
    case Insert_warn_attrib_upgrade:
      assert (false);
      break;
    case Insert_err_exists:
    case Insert_err_nomem:
      Kmem_alloc::allocator()->q_unaligned_free(ram_quota(),
          Config::PAGE_SIZE, mem);
      return 0;
    };

  unsigned long cap = cv & (Config::PAGE_SIZE - 1) | (unsigned long)mem;

  return reinterpret_cast<Entry*>(cap);
}

PRIVATE template< typename SPACE >
void
Generic_obj_space<SPACE>::caps_free()
{
  Mem_space *ms = mem_space();
  if (EXPECT_FALSE(!ms || !ms->dir()))
    return;

  Kmem_alloc *a = Kmem_alloc::allocator();
  for (unsigned long i = 0; i < map_max_address().value();
       i += Caps_per_page)
    {
      Entry *c = get_cap(i);
      if (!c)
	continue;

      Address cp = Address(ms->virt_to_phys(Address(c)));
      assert_kdb (cp != ~0UL);
      void *cv = (void*)Mem_layout::phys_to_pmem(cp);
      remove_dbg_info(cv);

      a->q_unaligned_free(ram_quota(), Config::PAGE_SIZE, cv);
    }
#if defined (CONFIG_ARM)
  ms->dir()->free_page_tables((void*)Mem_layout::Caps_start, (void*)Mem_layout::Caps_end, Kmem_alloc::q_allocator(ram_quota()));
#else
  ms->dir()->destroy(Virt_addr(Mem_layout::Caps_start),
                     Virt_addr(Mem_layout::Caps_end), Pdir::Depth - 1,
                     Kmem_alloc::q_allocator(ram_quota()));
#endif
}

//
// Utilities for map<Generic_obj_space> and unmap<Generic_obj_space>
//

PUBLIC  template< typename SPACE >
inline NEEDS[Generic_obj_space::cap_virt, Generic_obj_space::get_cap]
bool
Generic_obj_space<SPACE>::v_lookup(Addr const &virt, Phys_addr *phys = 0,
                                   Size *size = 0, unsigned *attribs = 0)
{
  if (size) size->set_value(1);
  Entry *cap;

  if (Optimize_local
      && mem_space() == Mem_space::current_mem_space(current_cpu()))
    cap = cap_virt(virt.value());
  else
    cap = get_cap(virt.value());

  if (EXPECT_FALSE(!cap))
    {
      if (size) size->set_value(Caps_per_page);
      return false;
    }

  if (Optimize_local)
    {
      Capability c = Mem_layout::read_special_safe((Capability*)cap);

      if (phys) *phys = c.obj();
      if (c.valid() && attribs) *attribs = c.rights();
      return c.valid();
    }
  else
    {
      Obj::set_entry(virt, cap);
      if (phys) *phys = cap->obj();
      if (cap->valid() && attribs) *attribs = cap->rights();
      return cap->valid();
    }
}

PUBLIC template< typename SPACE >
inline NEEDS [Generic_obj_space::cap_virt, Generic_obj_space::get_cap]
typename Generic_obj_space<SPACE>::Capability
Generic_obj_space<SPACE>::lookup(Address virt)
{
  Capability *c;
  virt &= ~(~0UL << Whole_space);

  if (mem_space() == Mem_space::current_mem_space(current_cpu()))
    c = reinterpret_cast<Capability*>(cap_virt(virt));
  else
    c = get_cap(virt);

  if (EXPECT_FALSE(!c))
    return Capability(0); // void

  return Mem_layout::read_special_safe(c);
}

PUBLIC template< typename SPACE >
inline NEEDS [Generic_obj_space::cap_virt]
Kobject_iface *
Generic_obj_space<SPACE>::lookup_local(Address virt, unsigned char *rights = 0)
{
  virt &= ~(~0UL << Whole_space);
  Capability *c = reinterpret_cast<Capability*>(cap_virt(virt));
  Capability cap = Mem_layout::read_special_safe(c);
  if (rights) *rights = cap.rights();
  return cap.obj();
}


PUBLIC template< typename SPACE >
inline NEEDS[<cassert>, Generic_obj_space::cap_virt, Generic_obj_space::get_cap]
unsigned long
Generic_obj_space<SPACE>::v_delete(Page_number virt, Size size,
                                   unsigned long page_attribs = L4_fpage::CRWSD)
{
  (void)size;
  assert (size.value() == 1);

  Entry *c;
  if (Optimize_local
      && mem_space() == Mem_space::current_mem_space(current_cpu()))
    {
      c = cap_virt(virt.value());
      if (!c)
	return 0;

      Capability cap = Mem_layout::read_special_safe((Capability*)c);
      if (!cap.valid())
	return 0;
    }
  else
    c = get_cap(virt.value());

  if (c && c->valid())
    {
      if (page_attribs & L4_fpage::R)
        c->invalidate();
      else
        c->del_rights(page_attribs & L4_fpage::CWSD);
    }

  return 0;
}

PUBLIC  template< typename SPACE >
inline NEEDS[Generic_obj_space::cap_virt, Generic_obj_space::caps_alloc,
             Generic_obj_space::alien_lookup, "kdb_ke.h"]
typename Generic_obj_space<SPACE>::Status
Generic_obj_space<SPACE>::v_insert(Phys_addr phys, Addr const &virt, Size size,
                                   unsigned char page_attribs)
{
  (void)size;
  assert (size.value() == 1);

  Entry *c;

  if (Optimize_local
      && mem_space() == Mem_space::current_mem_space(current_cpu()))
    {
      c = cap_virt(virt.value());
      if (!c)
	return Insert_err_nomem;

      Capability cap;
      if (!Mem_layout::read_special_safe((Capability*)c, cap)
	  && !caps_alloc(virt.value()))
	return Insert_err_nomem;
    }
  else
    {
      c = alien_lookup(virt.value());
      if (!c && !(c = caps_alloc(virt.value())))
	return Insert_err_nomem;
      Obj::set_entry(virt, c);
    }

  if (c->valid())
    {
      if (c->obj() == phys)
	{
	  if (EXPECT_FALSE(c->rights() == page_attribs))
	    return Insert_warn_exists;

	  c->add_rights(page_attribs);
	  return Insert_warn_attrib_upgrade;
	}
      else
	return Insert_err_exists;
    }

  c->set(phys, page_attribs);
  return Insert_ok;
}


PUBLIC  template< typename SPACE >
static inline
typename Generic_obj_space<SPACE>::Addr
Generic_obj_space<SPACE>::map_max_address()
{
  Mword r;

  r = (Mem_layout::Caps_end - Mem_layout::Caps_start) / sizeof(Entry);
  if (Map_max_address < r)
    r = Map_max_address;

  return Addr(r);
}


