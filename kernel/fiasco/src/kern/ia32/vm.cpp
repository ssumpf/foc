INTERFACE:

#include "task.h"

class Vm : public Task
{
public:
  explicit Vm(Ram_quota *q) : Task(q, Caps::mem() | Caps::obj()) {}
  int resume_vcpu(Context *, Vcpu_state *, bool) = 0;
};

template< typename VM >
struct Vm_allocator
{
  static Kmem_slab_t<VM> a;
};

template<typename VM>
Kmem_slab_t<VM> Vm_allocator<VM>::a("Vm");


// ------------------------------------------------------------------------
INTERFACE [obj_space_virt]:

#include "obj_space_phys_util.h"

EXTENSION class Vm : Obj_space_phys<Vm>
{
public:
  typedef Obj_space_phys<Vm> Vm_obj_space;
  using Task::ram_quota;
  static Ram_quota *ram_quota(Vm_obj_space const *obj_sp)
  { return static_cast<Vm const *>(obj_sp)->ram_quota(); }

  bool v_lookup(Obj_space::V_pfn const &virt, Obj_space::Phys_addr *phys,
                Obj_space::Page_order *size, Obj_space::Attr *attribs);

  L4_fpage::Rights v_delete(Obj_space::V_pfn virt, Order size,
                            L4_fpage::Rights page_attribs);
  Obj_space::Status v_insert(Obj_space::Phys_addr phys,
                             Obj_space::V_pfn const &virt,
                             Order size,
                             Obj_space::Attr page_attribs);

  Obj_space::Capability lookup(Cap_index virt);
  Obj_space::V_pfn obj_map_max_address() const;
  void caps_free();
};


// ------------------------------------------------------------------------
IMPLEMENTATION:

PUBLIC inline
Page_number
Vm::mem_space_map_max_address() const
{ return Page_number(1UL << (MWORD_BITS - Mem_space::Page_shift)); }

PUBLIC static
template< typename VM >
Slab_cache *
Vm::allocator()
{ return &Vm_allocator<VM>::a; }

// ------------------------------------------------------------------------
IMPLEMENTATION [ia32]:

PROTECTED static inline
bool
Vm::is_64bit()
{ return false; }

// ------------------------------------------------------------------------
IMPLEMENTATION [amd64]:

PROTECTED static inline
bool
Vm::is_64bit()
{ return true; }


//----------------------------------------------------------------------------
IMPLEMENTATION [obj_space_virt]:

//
// Utilities for map<Generic_obj_space> and unmap<Generic_obj_space>
//

IMPLEMENT
void FIASCO_FLATTEN
Vm::caps_free()
{ Vm_obj_space::caps_free(); }

IMPLEMENT
inline
bool FIASCO_FLATTEN
Vm::v_lookup(Obj_space::V_pfn const &virt, Obj_space::Phys_addr *phys,
             Obj_space::Page_order *size, Obj_space::Attr *attribs)
{ return Vm_obj_space::v_lookup(virt, phys, size, attribs); }

IMPLEMENT
inline
Obj_space::Capability FIASCO_FLATTEN
Vm::lookup(Cap_index virt)
{ return Vm_obj_space::lookup(virt); }

IMPLEMENT
inline
L4_fpage::Rights FIASCO_FLATTEN
Vm::v_delete(Obj_space::V_pfn virt, Obj_space::Page_order size,
             L4_fpage::Rights page_attribs)
{ return Vm_obj_space::v_delete(virt, size, page_attribs); }

IMPLEMENT
inline
Obj_space::Status FIASCO_FLATTEN
Vm::v_insert(Obj_space::Phys_addr phys, Obj_space::V_pfn const &virt,
             Obj_space::Page_order size, Obj_space::Attr page_attribs)
{ return (Obj_space::Status)Vm_obj_space::v_insert(phys, virt, size, page_attribs); }

IMPLEMENT
inline
Obj_space::V_pfn FIASCO_FLATTEN
Vm::obj_map_max_address() const
{ return Vm_obj_space::obj_map_max_address(); }

// ------------------------------------------------------------------------------
IMPLEMENTATION [obj_space_virt && debug]:

PUBLIC static inline
Vm *
Vm::get_space(Vm_obj_space *base)
{ return static_cast<Vm*>(base); }

PUBLIC virtual
Vm::Vm_obj_space::Entry *
Vm::jdb_lookup_cap(Cap_index index)
{ return Vm_obj_space::jdb_lookup_cap(index); }

// ------------------------------------------------------------------------------
IMPLEMENTATION [obj_space_virt && !debug]:

PUBLIC static inline
Vm *
Vm::get_space(Vm_obj_space *)
{ return 0; }
