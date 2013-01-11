INTERFACE[ia32,ux]:

#include <cassert>
#include "types.h"
#include "config.h"
#include "ptab_base.h"

class PF {};

class Pte_base
{
public:
  typedef Mword Raw;

  enum
  {
    Super_level   = 0,
    Valid         = 0x00000001, ///< Valid
    Writable      = 0x00000002, ///< Writable
    User          = 0x00000004, ///< User accessible
    Write_through = 0x00000008, ///< Write through
    Cacheable     = 0x00000000, ///< Cache is enabled
    Noncacheable  = 0x00000010, ///< Caching is off
    Referenced    = 0x00000020, ///< Page was referenced
    Dirty         = 0x00000040, ///< Page was modified
    Pse_bit       = 0x00000080, ///< Indicates a super page
    Cpu_global    = 0x00000100, ///< pinned in the TLB
    L4_global     = 0x00000200, ///< pinned in the TLB
    Pfn           = 0xfffff000, ///< page frame number
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



typedef Ptab::List< Ptab::Traits<Pd_entry, 22, 10, true, false>,
                    Ptab::Traits<Pt_entry, 12, 10, true> > Ptab_traits;

typedef Ptab::Shift<Ptab_traits, Virt_addr::Shift>::List Ptab_traits_vpn;
typedef Ptab::Page_addr_wrap<Page_number, Virt_addr::Shift> Ptab_va_vpn;
