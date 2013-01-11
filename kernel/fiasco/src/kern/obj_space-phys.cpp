INTERFACE:

#include "assert_opt.h"

EXTENSION class Generic_obj_space
{
private:
  enum
  {
    Slots_per_dir = Config::PAGE_SIZE / sizeof(void*)
  };

  struct Cap_table { Entry e[Caps_per_page]; };
  struct Cap_dir   { Cap_table *d[Slots_per_dir]; };
  Cap_dir *_dir;

  Ram_quota *ram_quota() const
  {
    assert_opt (this);
    return static_cast<SPACE const *>(this)->ram_quota();
  }
};


//----------------------------------------------------------------------------
IMPLEMENTATION:

#include <cstring>
#include <cassert>

#include "atomic.h"
#include "config.h"
#include "cpu.h"
#include "kdb_ke.h"
#include "kmem_alloc.h"
#include "mem.h"
#include "mem_layout.h"
#include "ram_quota.h"
#include "static_assert.h"


PUBLIC template< typename SPACE >
inline NEEDS["static_assert.h"]
Generic_obj_space<SPACE>::Generic_obj_space()
{
  static_assert(sizeof(Cap_dir) == Config::PAGE_SIZE, "cap_dir size mismatch");
  _dir = (Cap_dir*)Kmem_alloc::allocator()->q_unaligned_alloc(ram_quota(), Config::PAGE_SIZE);
  if (_dir)
    Mem::memset_mwords(_dir, 0, Config::PAGE_SIZE / sizeof(Mword));
}

PRIVATE template< typename SPACE >
typename Generic_obj_space<SPACE>::Entry *
Generic_obj_space<SPACE>::get_cap(Address index)
{
  if (EXPECT_FALSE(!_dir))
    return 0;

  unsigned d_idx = index / Caps_per_page;
  if (EXPECT_FALSE(d_idx >= Slots_per_dir))
    return 0;

  Cap_table *tab = _dir->d[d_idx];

  if (EXPECT_FALSE(!tab))
    return 0;

  unsigned offs  = index % Caps_per_page;
  return &tab->e[offs];
}

PRIVATE template< typename SPACE >
typename Generic_obj_space<SPACE>::Entry *
Generic_obj_space<SPACE>::caps_alloc(Address virt)
{
  static_assert(sizeof(Cap_table) == Config::PAGE_SIZE, "cap table size mismatch");
  unsigned d_idx = virt / Caps_per_page;
  if (EXPECT_FALSE(d_idx >= Slots_per_dir))
    return 0;

  void *mem = Kmem_alloc::allocator()->q_unaligned_alloc(ram_quota(), Config::PAGE_SIZE);

  if (!mem)
    return 0;

  add_dbg_info(mem, this, virt);

  Mem::memset_mwords(mem, 0, Config::PAGE_SIZE / sizeof(Mword));



  Cap_table *tab = _dir->d[d_idx] = (Cap_table*)mem;

  return &tab->e[virt % Caps_per_page];
}

PRIVATE template< typename SPACE >
void
Generic_obj_space<SPACE>::caps_free()
{ 
  if (!_dir)
    return;

  Cap_dir *d = _dir;
  _dir = 0;

  Kmem_alloc *a = Kmem_alloc::allocator();
  for (unsigned i = 0; i < Slots_per_dir; ++i)
    {
      if (!d->d[i])
        continue;

      remove_dbg_info(d->d[i]);
      a->q_unaligned_free(ram_quota(), Config::PAGE_SIZE, d->d[i]);
    }

  a->q_unaligned_free(ram_quota(), Config::PAGE_SIZE, d);
}

//
// Utilities for map<Generic_obj_space> and unmap<Generic_obj_space>
//

PUBLIC template< typename SPACE >
inline NEEDS[Generic_obj_space::get_cap]
bool
Generic_obj_space<SPACE>::v_lookup(Addr const &virt, Phys_addr *phys = 0,
                                   Size *size = 0, unsigned *attribs = 0)
{
  if (size) *size = Size::create(1);
  Entry *cap = get_cap(virt.value());

  if (EXPECT_FALSE(!cap))
    {
      if (size) *size = Size::create(Caps_per_page);
      return false;
    }

  Capability c = *cap;

  Obj::set_entry(virt, cap);
  if (phys) *phys = c.obj();
  if (c.valid() && attribs) *attribs = cap->rights();
  return c.valid();
}

PUBLIC template< typename SPACE >
inline NEEDS [Generic_obj_space::get_cap]
typename Generic_obj_space<SPACE>::Capability
Generic_obj_space<SPACE>::lookup(Address virt)
{
  Capability *c = get_cap(virt);

  if (EXPECT_FALSE(!c))
    return Capability(0); // void

  return *c;
}

PUBLIC template< typename SPACE >
inline
Kobject_iface *
Generic_obj_space<SPACE>::lookup_local(Address virt, unsigned char *rights = 0)
{
  Entry *c = get_cap(virt);

  if (EXPECT_FALSE(!c))
    return 0;

  Capability cap = *c;

  if (rights)
    *rights = cap.rights();

  return cap.obj();
}


PUBLIC template< typename SPACE >
inline NEEDS[<cassert>, Generic_obj_space::get_cap]
unsigned long
Generic_obj_space<SPACE>::v_delete(Page_number virt, Size size,
                                   unsigned long page_attribs = L4_fpage::RWX)
{
  (void)size;
  assert (size.value() == 1);
  Capability *c = get_cap(virt.value());

  if (c && c->valid())
    {
      if (page_attribs & L4_fpage::R)
        c->invalidate();
      else
	c->del_rights(page_attribs & L4_fpage::WX);
    }

  return 0;
}

PUBLIC template< typename SPACE >
inline NEEDS[Generic_obj_space::caps_alloc, "kdb_ke.h"]
typename Generic_obj_space<SPACE>::Status
Generic_obj_space<SPACE>::v_insert(Phys_addr phys, Addr const &virt, Size size,
                                   unsigned char page_attribs)
{
  (void)size;
  assert (size.value() == 1);

  Entry *c = get_cap(virt.value());

  if (!c && !(c = caps_alloc(virt.value())))
    return Insert_err_nomem;

  assert (size.value() == 1);

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

  Obj::set_entry(virt, c);
  c->set(phys, page_attribs);
  return Insert_ok;
}


PUBLIC template< typename SPACE >
static inline
typename Generic_obj_space<SPACE>::Addr
Generic_obj_space<SPACE>::map_max_address()
{
  return Addr(Slots_per_dir * Caps_per_page);
}

