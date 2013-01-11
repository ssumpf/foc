INTERFACE:

#include "kobject_mapdb.h"


IMPLEMENTATION:

#include "assert.h"
#include "assert_opt.h"

#include "obj_space.h"
#include "l4_types.h"
#include "mappable.h"

inline NEEDS["assert_opt.h"]
void
save_access_attribs (Kobject_mapdb* /*mapdb*/,
    const Kobject_mapdb::Frame& /*mapdb_frame*/,
    Kobject_mapdb::Mapping* /*mapping*/, Obj_space* /*space*/,
    unsigned /*page_rights*/,
    Obj_space::Addr /*virt*/, Obj_space::Phys_addr /*phys*/, Obj_space::Size /*size*/,
    bool /*me_too*/)
{}

L4_error
obj_map(Space *from, L4_fpage const &fp_from,
        Space *to, L4_fpage const &fp_to, L4_msg_item control,
        Kobject ***reap_list)
{
  assert_opt (from);
  assert_opt (to);

  typedef Map_traits<Obj_space> Mt;
  Mt::Addr rcv_addr = Mt::get_addr(fp_to);
  Mt::Addr snd_addr = Mt::get_addr(fp_from);
  Mt::Size snd_size = Mt::Size::from_shift(fp_from.order());
  Mt::Size rcv_size = Mt::Size::from_shift(fp_to.order());
  Mt::Addr offs(control.index());

  snd_addr = snd_addr.trunc(snd_size);
  rcv_addr = rcv_addr.trunc(rcv_size);

  Mt::constraint(snd_addr, snd_size, rcv_addr, rcv_size, offs);

  if (snd_size == 0)
    return L4_error::None;

  unsigned long del_attribs, add_attribs;
  Mt::attribs(control, fp_from, &del_attribs, &add_attribs);

  return map<Obj_space>((Kobject_mapdb*)0,
	      from, from, snd_addr,
	      snd_size,
	      to, to, rcv_addr,
	      control.is_grant(), add_attribs, del_attribs,
	      reap_list);
}

unsigned __attribute__((nonnull(1)))
obj_fpage_unmap(Space * space, L4_fpage fp, L4_map_mask mask,
                Kobject ***reap_list)
{
  assert_opt (space);
  typedef Map_traits<Obj_space> Mt;
  Mt::Size size = Mt::Size::from_shift(fp.order());
  Mt::Addr addr = Mt::get_addr(fp);
  addr = addr.trunc(size);

  // XXX prevent unmaps when a task has no caps enabled

  return unmap<Obj_space>((Kobject_mapdb*)0, space, space,
               addr, size,
               fp.rights(), mask, reap_list);
}


L4_error
obj_map(Space *from, unsigned long snd_addr, unsigned long snd_size,
        Space *to, unsigned long rcv_addr,
        Kobject ***reap_list, bool grant = false,
        unsigned attrib_add = 0, unsigned attrib_del = 0)
{
  assert_opt (from);
  assert_opt (to);
  typedef Map_traits<Obj_space> Mt;

  return map<Obj_space>((Kobject_mapdb*)0,
	     from, from, Mt::Addr(snd_addr),
	     Mt::Size(snd_size),
	     to, to, Mt::Addr(rcv_addr),
	     grant, attrib_add, attrib_del, reap_list);
}

bool
map(Kobject_iface *o, Obj_space* to, Space *to_id, Address _rcv_addr,
    Kobject ***reap_list, unsigned attribs = L4_fpage::CRWSD)
{
  assert_opt (o);
  assert_opt (to);
  assert_opt (to_id);

  typedef Obj_space SPACE;
  typedef Obj_space::Addr Addr;
  typedef Obj_space::Size Size;

  Page_number const rcv_addr = Page_number::create(_rcv_addr);

  if (EXPECT_FALSE(rcv_addr >= to->map_max_address()))
    return 0;

  // Receiver lookup.
  Obj_space::Phys_addr r_phys;
  Size r_size;
  unsigned r_attribs;

  Addr ra(rcv_addr);
  if (to->v_lookup(ra, &r_phys, &r_size, &r_attribs))
    unmap((Kobject_mapdb*)0, to, to_id, ra, r_size,
          L4_fpage::RWX, L4_map_mask::full(), reap_list);

  attribs &= L4_fpage::WX;
  // Do the actual insertion.
  Obj_space::Status status
    = to->v_insert(o, ra, Size::create(1), attribs);

  switch (status)
    {
    case SPACE::Insert_warn_exists:
    case SPACE::Insert_warn_attrib_upgrade:
    case SPACE::Insert_ok:

      if (status == SPACE::Insert_ok)
	{
	  if (! o->map_root()->insert(0, to_id, ra))
	    {
	      to->v_delete(rcv_addr, Obj_space::Size::create(1));
	      return 0;
	    }
	}
      break;

    case SPACE::Insert_err_nomem:
      return 0;

    case SPACE::Insert_err_exists:
      // Do not flag an error here -- because according to L4
      // semantics, it isn't.
      break;
    }

  return 1;
}
