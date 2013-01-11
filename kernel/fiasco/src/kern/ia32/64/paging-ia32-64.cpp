INTERFACE[amd64]:

#include <cassert>
#include "types.h"
#include "config.h"
#include "mem_layout.h"
#include "ptab_base.h"

class PF {};

class Pte_base
{
public:
  typedef Mword Raw;

  enum
  {
    Super_level   = 2,
    Valid         = 0x00000001LL, ///< Valid
    Writable      = 0x00000002LL, ///< Writable
    User          = 0x00000004LL, ///< User accessible
    Write_through = 0x00000008LL, ///< Write through
    Cacheable     = 0x00000000LL, ///< Cache is enabled
    Noncacheable  = 0x00000010LL, ///< Caching is off
    Referenced    = 0x00000020LL, ///< Page was referenced
    Dirty         = 0x00000040LL, ///< Page was modified
    Pse_bit       = 0x00000080LL, ///< Indicates a super page
    Cpu_global    = 0x00000100LL, ///< pinned in the TLB
    L4_global     = 0x00000200LL, ///< pinned in the TLB
    Pfn           = 0x000ffffffffff000LL, ///< page frame number
  };

  Mword addr() const { return _raw & Pfn; }

protected:
  Raw _raw;

private:
  static Unsigned32 _cpu_global;
};

class Pt_entry : public Pte_base
{
public:
  enum { Page_shift = Config::PAGE_SHIFT };
  Mword leaf() const { return true; }
  void set(Address p, bool intermed, bool present, unsigned long attrs = 0) 
  {
    _raw = (p & Pfn) | (present ? 1 : 0)
      | (intermed ? (Writable | User | Cacheable) : 0) | attrs;
  }
};

class Pd_entry : public Pte_base
{
public:
  enum { Page_shift = Config::SUPERPAGE_SHIFT };
  Mword leaf() const { return _raw & Pse_bit; }
  void set(Address p, bool intermed, bool present, unsigned long attrs = 0)
  {
    _raw = (p & Pfn) | (present ? 1 : 0)
      | (intermed ? (Writable | User | Cacheable) : Pse_bit) | attrs;
  }
};

class Pdp_entry : public Pte_base
{
public:
  // this is just a dummy, because there are no real pages on level 0 and 1
  enum { Page_shift = 0 };
  Mword leaf() const { return false; }
  void set(Address p, bool, bool present, unsigned long attrs = 0)
  {
    _raw = (p & Pfn) | (present ? 1 : 0)
      | (Writable | User | Cacheable) | attrs;
  }
};


typedef Ptab::List< Ptab::Traits<Pdp_entry, 39, 9, false>,
        Ptab::List< Ptab::Traits<Pdp_entry, 30, 9, false>,
        Ptab::List< Ptab::Traits<Pd_entry,  21, 9, true>,
                    Ptab::Traits<Pt_entry,  12, 9, true> > > > Ptab_traits;

typedef Ptab::Shift<Ptab_traits, Virt_addr::Shift>::List Ptab_traits_vpn;
typedef Ptab::Page_addr_wrap<Page_number, Virt_addr::Shift> Ptab_va_vpn;
