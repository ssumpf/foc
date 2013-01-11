INTERFACE:

#include "types.h"

namespace Page {
  
  /* These things must be defined in arch part in
     the most efficent way according to the architecture.
  
  typedef int Attribs;

  enum Attribs_enum {
    USER_NO  = xxx, ///< User No access
    USER_RO  = xxx, ///< User Read only
    USER_RW  = xxx, ///< User Read/Write
    USER_RX  = xxx, ///< User Read/Execute
    USER_XO  = xxx, ///< User Execute only
    USER_RWX = xxx, ///< User Read/Write/Execute

    NONCACHEABLE = xxx, ///< Caching is off
    CACHEABLE    = xxx, ///< Cahe is enabled
  };
     
  
  */


};

EXTENSION class PF {
public:
  static Mword is_translation_error( Mword error );
  static Mword is_usermode_error( Mword error );
  static Mword is_read_error( Mword error );
  static Mword addr_to_msgword0( Address pfa, Mword error );
  static Mword pc_to_msgword1( Address pc, Mword error );
};


class Pdir_alloc_null
{
public:
  Pdir_alloc_null() {}
  void *alloc(unsigned long) const { return 0; }
  void free(void *, unsigned long) const {}
  bool valid() const { return false; }
};

template<typename ALLOC>
class Pdir_alloc_simple
{
public:
  Pdir_alloc_simple(ALLOC *a = 0) : _a(a) {}

  void *alloc(unsigned long size) const
  { return _a->unaligned_alloc(size); }

  void free(void *block, unsigned long size) const
  { return _a->unaligned_free(size, block); }

  bool valid() const { return _a; }

private:
  ALLOC *_a;
};


class Pdir : public Ptab::Base<Pte_base, Ptab_traits_vpn, Ptab_va_vpn >
{
public:
  enum { Super_level = Pte_base::Super_level };

private:
  static bool       _have_superpages;
  static unsigned   _super_level;
};

IMPLEMENTATION:
//---------------------------------------------------------------------------

bool Pdir::_have_superpages;
unsigned  Pdir::_super_level;

template<typename ALLOC>
inline Pdir_alloc_simple<ALLOC> pdir_alloc(ALLOC *a)
{ return Pdir_alloc_simple<ALLOC>(a); }

PUBLIC static inline
void
Pdir::have_superpages(bool yes)
{
  _have_superpages = yes;
  _super_level = yes ? Super_level : (Super_level + 1);
}

PUBLIC static inline
unsigned
Pdir::super_level()
{ return _super_level; }


IMPLEMENT inline NEEDS[PF::is_usermode_error]
Mword PF::pc_to_msgword1(Address pc, Mword error)
{
  return is_usermode_error(error) ? pc : (Mword)-1;
}

//---------------------------------------------------------------------------
IMPLEMENTATION[!ppc32]:

PUBLIC
Address
Pdir::virt_to_phys(Address virt) const
{
  Iter i = walk(Virt_addr(virt));
  if (!i.e->valid())
    return ~0;

  return i.e->addr() | (virt & ~(~0UL << i.shift()));
}

//---------------------------------------------------------------------------
IMPLEMENTATION[ppc32]:

PUBLIC
Address
Pdir::virt_to_phys(Address virt) const
{
  Iter i = walk(Virt_addr(virt));

  if (!i.e->valid())
    return ~0UL;

  Address phys;
  Pte_htab::pte_lookup(i.e, &phys);
  return phys | (virt & ~(~0UL << i.shift()));
}
