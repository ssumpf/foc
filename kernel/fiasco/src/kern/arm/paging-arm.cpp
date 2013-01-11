INTERFACE [arm]:


class PF {};

//-----------------------------------------------------------------------------
INTERFACE [arm && armv5]:

#include "types.h"


namespace Page
{
  typedef Unsigned32 Attribs;

  enum Attribs_enum
  {
    KERN_RW  = 0x0400, ///< User No access
    USER_RO  = 0x0000, ///< User Read only
    USER_RW  = 0x0c00, ///< User Read/Write

    USER_BIT = 0x00800,

    Cache_mask    = 0x0c,
    NONCACHEABLE  = 0x00, ///< Caching is off
    CACHEABLE     = 0x0c, ///< Cache is enabled

    // The next are ARM specific
    WRITETHROUGH = 0x08, ///< Write through cached
    BUFFERED     = 0x04, ///< Write buffer enabled

    MAX_ATTRIBS  = 0x0dec,
    Local_page   = 0,
  };
};





//----------------------------------------------------------------------------
INTERFACE [arm && (armv6 || armv7)]:

#include "types.h"

namespace Page
{
  typedef Unsigned32 Attribs;

  enum Attribs_enum
  {
    KERN_RO  = 0x0210,
    KERN_RW  = 0x0010, ///< User No access
    USER_RO  = 0x0230, ///< User Read only
    USER_RW  = 0x0030, ///< User Read/Write

    USER_BIT = 0x0020,

    Cache_mask    = 0x1cc,
    NONCACHEABLE  = 0x000, ///< Caching is off
    CACHEABLE     = 0x144, ///< Cache is enabled

    // The next are ARM specific
    WRITETHROUGH = 0x08, ///< Write through cached
    BUFFERED     = 0x40, ///< Write buffer enabled -- Normal, non-cached

    MAX_ATTRIBS  = 0x0ffc,
    Local_page   = 0x800,
  };
};


//-----------------------------------------------------------------------------
INTERFACE [arm]:

#include "ptab_base.h"


// dummy for JDB
class Pte_base
{
public:
  enum
  {
    Super_level    = 0,
    Valid          = 0x3,
    Type_mask      = 0x3,
    Type_1MB       = 0x2,
    Type_4KB       = 0x2,
    Type_coarse_pt = 0x1,
  };
  typedef Mword Raw;
  Raw raw() const  { return _raw; }
  Address addr() const { return _raw & (~0UL << 12); }

protected:
  Raw _raw;
};

class Pd_entry : public Pte_base
{
public:
  enum { Page_shift = Config::SUPERPAGE_SHIFT };
  Mword leaf() const { return (_raw & Type_mask) == Type_1MB; }
  void set(Address p, bool intermed, bool present, unsigned long attrs = 0)
  {
    _raw = (p & (~0UL << Page_shift))
      | (present ? (intermed ? Type_coarse_pt : Type_1MB) : 0) | attrs;
  }
};

class Pt_entry : public Pte_base
{
public:
  enum { Page_shift = Config::PAGE_SHIFT };
  Mword leaf() const { return true; }
  void set(Address p, bool, bool present, unsigned long attrs = 0)
  {
    _raw = (p & (~0UL << Page_shift)) | (present ? Type_4KB : 0) | attrs;
  }
};

typedef Ptab::List< Ptab::Traits< Pd_entry, 20, 12, true>,
                    Ptab::Traits< Pt_entry, 12, 8, true> > Ptab_traits;

typedef Ptab_traits Ptab_traits_vpn;
typedef Ptab::Page_addr_wrap<Virt_addr, 0> Ptab_va_vpn;

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv5]:

PUBLIC static inline
Mword PF::is_alignment_error(Mword error)
{ return (error & 0xf0000d) == 0x400001; }

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && (armv6 || armv7)]:

PUBLIC static inline
Mword PF::is_alignment_error(Mword error)
{ return (error & 0xf0040f) == 0x400001; }

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

PUBLIC inline
void
Pte_base::clear()
{ _raw = 0; }

PUBLIC inline
int
Pte_base::valid() const
{
  return _raw & Valid;
}

IMPLEMENT inline
Mword PF::is_translation_error( Mword error )
{
  return (error & 0x0d/*FSR_STATUS_MASK*/) == 0x05/*FSR_TRANSL*/;
}

IMPLEMENT inline
Mword PF::is_usermode_error( Mword error )
{
  return (error & 0x00010000/*PF_USERMODE*/);
}

IMPLEMENT inline
Mword PF::is_read_error( Mword error )
{
  return !(error & (1UL << 11));
}

IMPLEMENT inline
Mword PF::addr_to_msgword0( Address pfa, Mword error )
{
  Mword a = pfa & ~3;
  if(is_translation_error( error ))
    a |= 1;
  if(!is_read_error(error))
    a |= 2;
  return a;
}

