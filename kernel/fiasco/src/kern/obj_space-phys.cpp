INTERFACE:

#include "assert_opt.h"
#include "obj_space_phys_util.h"

EXTENSION class Generic_obj_space : Obj_space_phys<Generic_obj_space< SPACE > >
{
private:
  typedef Obj_space_phys<Generic_obj_space< SPACE > > Base;

public:
  static Ram_quota *ram_quota(Base const *base)
  {
    assert_opt (base);
    return static_cast<SPACE const *>(base)->ram_quota();
  }
};


//----------------------------------------------------------------------------
IMPLEMENTATION:

//
// Utilities for map<Generic_obj_space> and unmap<Generic_obj_space>
//

IMPLEMENT template< typename SPACE >
inline
bool __attribute__((__flatten__))
Generic_obj_space<SPACE>::v_lookup(V_pfn const &virt, Phys_addr *phys,
                                   Page_order *size, Attr *attribs)
{ return Base::v_lookup(virt, phys, size, attribs); }

IMPLEMENT template< typename SPACE >
inline __attribute__((__flatten__))
typename Generic_obj_space<SPACE>::Capability __attribute__((__flatten__))
Generic_obj_space<SPACE>::lookup(Cap_index virt)
{ return Base::lookup(virt); }

IMPLEMENT template< typename SPACE >
inline __attribute__((__flatten__))
Kobject_iface *
Generic_obj_space<SPACE>::lookup_local(Cap_index virt, L4_fpage::Rights *rights = 0)
{ return Base::lookup_local(virt, rights); }

IMPLEMENT template< typename SPACE >
inline __attribute__((__flatten__))
L4_fpage::Rights __attribute__((__flatten__))
Generic_obj_space<SPACE>::v_delete(V_pfn virt, Page_order size,
                                   L4_fpage::Rights page_attribs)
{ return Base::v_delete(virt, size, page_attribs); }

IMPLEMENT template< typename SPACE >
inline __attribute__((__flatten__))
typename Generic_obj_space<SPACE>::Status __attribute__((__flatten__))
Generic_obj_space<SPACE>::v_insert(Phys_addr phys, V_pfn const &virt, Page_order size,
                                   Attr page_attribs)
{ return (Status)Base::v_insert(phys, virt, size, page_attribs); }

IMPLEMENT template< typename SPACE >
inline __attribute__((__flatten__))
typename Generic_obj_space<SPACE>::V_pfn
Generic_obj_space<SPACE>::obj_map_max_address() const
{ return Base::obj_map_max_address(); }

// ------------------------------------------------------------------------------
IMPLEMENTATION [debug]:

PUBLIC template< typename SPACE > static inline
SPACE *
Generic_obj_space<SPACE>::get_space(Base *base)
{ return static_cast<SPACE*>(base); }

IMPLEMENT template< typename SPACE > inline
typename Generic_obj_space<SPACE>::Entry *
Generic_obj_space<SPACE>::jdb_lookup_cap(Cap_index index)
{ return Base::jdb_lookup_cap(index); }

// ------------------------------------------------------------------------------
IMPLEMENTATION [!debug]:

PUBLIC template< typename SPACE > static inline
SPACE *
Generic_obj_space<SPACE>::get_space(Base *)
{ return 0; }
