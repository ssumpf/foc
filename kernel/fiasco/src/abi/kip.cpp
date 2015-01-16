INTERFACE:

class Mem_desc
{
public:
  enum Mem_type
  {
    Undefined    = 0x0,
    Conventional = 0x1,
    Reserved     = 0x2,
    Dedicated    = 0x3,
    Shared       = 0x4,
    Kernel_tmp   = 0x7,

    Info         = 0xd,
    Bootloader   = 0xe,
    Arch         = 0xf,
  };

  enum Ext_type_info
  {
    Info_acpi_rsdp = 0
  };

private:
  Mword _l, _h;
};

//----------------------------------------------------------------------------
INTERFACE [ia32]:

/* Empty class for VHW descriptor in KIP for native ia32 */
class Vhw_descriptor {};

//----------------------------------------------------------------------------
INTERFACE:

#include "mem_region.h"
#include "types.h"

class Kip
{
public:
  void print() const;

  char const *version_string() const;

  // returns the 1st address beyond all available physical memory
  Address main_memory_high() const;

private:
  static Kip *global_kip asm ("GLOBAL_KIP");
};

#define L4_KERNEL_INFO_MAGIC (0x4BE6344CL) /* "L4µK" */

//============================================================================
IMPLEMENTATION:

#include "assert.h"
#include "config.h"
#include "panic.h"
#include "static_assert.h"
#include "version.h"

PUBLIC inline
Mem_desc::Mem_desc(Address start, Address end, Mem_type t, bool v = false,
                   unsigned st = 0)
: _l((start & ~0x3ffUL) | (t & 0x0f) | ((st << 4) & 0x0f0)
     | (v?0x0200:0x0)),
  _h(end)
{}

PUBLIC inline ALWAYS_INLINE
Address Mem_desc::start() const
{ return _l & ~0x3ffUL; }

PUBLIC inline ALWAYS_INLINE
Address Mem_desc::end() const
{ return _h | 0x3ffUL; }

PUBLIC inline ALWAYS_INLINE
void
Mem_desc::type(Mem_type t)
{ _l = (_l & ~0x0f) | (t & 0x0f); }

PUBLIC inline ALWAYS_INLINE
Mem_desc::Mem_type Mem_desc::type() const
{ return (Mem_type)(_l & 0x0f); }

PUBLIC inline
unsigned Mem_desc::ext_type() const
{ return (_l >> 4) & 0x0f; }

PUBLIC inline ALWAYS_INLINE
unsigned Mem_desc::is_virtual() const
{ return _l & 0x200; }

PUBLIC inline
bool Mem_desc::contains(unsigned long addr)
{
  return start() <= addr && end() >= addr;
}

PUBLIC inline ALWAYS_INLINE
bool Mem_desc::valid() const
{ return type() && start() < end(); }

PUBLIC inline ALWAYS_INLINE
Mem_desc *Kip::mem_descs()
{ return (Mem_desc*)(((Address)this) + (_mem_info >> (MWORD_BITS/2))); }

PUBLIC inline
Mem_desc const *Kip::mem_descs() const
{ return (Mem_desc const *)(((Address)this) + (_mem_info >> (MWORD_BITS/2))); }

PUBLIC inline ALWAYS_INLINE
unsigned Kip::num_mem_descs() const
{ return _mem_info & ((1UL << (MWORD_BITS/2))-1); }

PUBLIC inline
void Kip::num_mem_descs (unsigned n)
{
  _mem_info = (_mem_info & ~((1UL << (MWORD_BITS/2))-1)
	       | (n & ((1UL << (MWORD_BITS/2))-1)));
}

PUBLIC
Mem_desc *Kip::add_mem_region(Mem_desc const &md)
{
  Mem_desc *m = mem_descs();
  Mem_desc *end = m + num_mem_descs();
  for (;m<end;++m)
    {
      if (m->type() == Mem_desc::Undefined)
	{
	  *m = md;
	  return m;
	}
    }

  // Add mem region failed -- must be a Fiasco startup problem.  Bail out.
  panic("Too few memory descriptors in KIP");

  return 0;
}

Kip *Kip::global_kip;

PUBLIC static inline ALWAYS_INLINE NEEDS["config.h"]
void
Kip::init_global_kip(Kip *kip)
{
  global_kip = kip;

  kip->platform_info.is_mp = Config::Max_num_cpus > 1;

  // check that the KIP has actually been set up
  //assert(kip->sigma0_ip && kip->root_ip && kip->user_ptr);
}

PUBLIC static inline Kip *Kip::k() { return global_kip; }

IMPLEMENT
char const *Kip::version_string() const
{
  static_assert((sizeof(Kip) & 0xf) == 0, "Invalid KIP structure size");

  return reinterpret_cast <char const *> (this) + (offset_version_strings << 4);
}

asm(".section .initkip.version, \"a\", %progbits        \n"	\
    ".string \"" CONFIG_KERNEL_VERSION_STRING "\"       \n"	\
    ".previous                                          \n");

asm(".section .initkip.features.fini, \"a\", %progbits  \n"	\
    ".string \"\"                                       \n"	\
    ".previous                                          \n");
