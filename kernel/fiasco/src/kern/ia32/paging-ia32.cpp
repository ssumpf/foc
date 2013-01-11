INTERFACE [ia32 || amd64 || ux]:

#include "types.h"
#include "config.h"


namespace Page
{
  typedef Unsigned32 Attribs;

  enum Attribs_enum
  {
    KERN_RW       = 0x00000002, ///< User No access
    USER_RO       = 0x00000004, ///< User Read only
    USER_RW       = 0x00000006, ///< User Read/Write
    USER_RX       = 0x00000004, ///< User Read/Execute
    USER_XO       = 0x00000004, ///< User Execute only
    USER_RWX      = 0x00000006, ///< User Read/Write/Execute
    MAX_ATTRIBS   = 0x00000006,
    Cache_mask    = 0x00000018, ///< Cache attrbute mask
    CACHEABLE     = 0x00000000,
    BUFFERED      = 0x00000010,
    NONCACHEABLE  = 0x00000018,
  };
};

EXTENSION class Pte_base {};


//---------------------------------------------------------------------------
IMPLEMENTATION [ia32 || amd64 || ux]:

PUBLIC inline
Pte_base &
Pte_base::operator = (Pte_base const &other)
{
  _raw = other.raw();
  return *this;
}

PUBLIC inline
Pte_base &
Pte_base::operator = (Mword raw)
{
  _raw = raw;
  return *this;
}

PUBLIC inline
Mword
Pte_base::raw() const
{
  return _raw;
}

PUBLIC inline
void
Pte_base::add_attr(Mword attr)
{
  _raw |= attr;
}

PUBLIC inline
void
Pte_base::del_attr(Mword attr)
{
  _raw &= ~attr;
}

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

PUBLIC inline
int
Pte_base::writable() const
{
  return _raw & Writable;
}

PUBLIC inline
Address
Pt_entry::pfn() const
{
  return _raw & Pfn;
}

Unsigned32 Pte_base::_cpu_global = Pte_base::L4_global;

PUBLIC static inline
void
Pte_base::enable_global()
{
  _cpu_global |= Pte_base::Cpu_global;
}

/**
 * Global entries are entries that are not automatically flushed when the
 * page-table base register is reloaded. They are intended for kernel data
 * that is shared between all tasks.
 * @return global page-table--entry flags
 */
PUBLIC static inline
Unsigned32
Pte_base::global()
{
  return _cpu_global;
}


//--------------------------------------------------------------------------
#include "cpu.h"
#include "mem_layout.h"
#include "regdefs.h"

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::is_translation_error(Mword error)
{
  return !(error & PF_ERR_PRESENT);
}

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::is_usermode_error(Mword error)
{
  return (error & PF_ERR_USERMODE);
}

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::is_read_error(Mword error)
{
  return !(error & PF_ERR_WRITE);
}

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::addr_to_msgword0(Address pfa, Mword error)
{
  return    (pfa   & ~(PF_ERR_PRESENT | PF_ERR_WRITE))
          | (error &  (PF_ERR_PRESENT | PF_ERR_WRITE));
}

