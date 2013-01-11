INTERFACE:

#include <types.h>

class Acpi_gas
{
public:
  enum Type { System_mem = 0, System_io = 1, Pci_cfg_mem = 2 };
  Unsigned8  id;
  Unsigned8  width;
  Unsigned8  offset;
  Unsigned8  access_size;
  Unsigned64 addr;
} __attribute__((packed));



class Acpi_table_head
{
public:
  char       signature[4];
  Unsigned32 len;
  Unsigned8  rev;
  Unsigned8  chk_sum;
  char       oem_id[6];
  char       oem_tid[8];
  Unsigned32 oem_rev;
  Unsigned32 creator_id;
  Unsigned32 creator_rev;

  bool checksum_ok() const;
} __attribute__((packed));


template< typename T >
class Acpi_sdt : public Acpi_table_head
{
public:
  T ptrs[0];

} __attribute__((packed));

typedef Acpi_sdt<Unsigned32> Acpi_rsdt;
typedef Acpi_sdt<Unsigned64> Acpi_xsdt;

class Acpi_rsdp
{
public:
  char       signature[8];
  Unsigned8  chk_sum;
  char       oem[6];
  Unsigned8  rev;
  Unsigned32 rsdt_phys;
  Unsigned32 len;
  Unsigned64 xsdt_phys;
  Unsigned8  ext_chk_sum;
  char       reserved[3];

  Acpi_rsdt const *rsdt() const;
  Acpi_xsdt const *xsdt() const;

  bool checksum_ok() const;

  static Acpi_rsdp const *locate();
} __attribute__((packed));

class Acpi
{
public:
  static Acpi_rsdt const *rsdt() { return _rsdt; }
  static Acpi_xsdt const *xsdt() { return _xsdt; }

private:
  static Acpi_rsdt const *_rsdt;
  static Acpi_xsdt const *_xsdt;
  static bool _init_done;
};

class Acpi_madt : public Acpi_table_head
{
public:
  enum Type
  { LAPIC, IOAPIC, Irq_src_ovr, NMI, LAPIC_NMI, LAPIC_adr_ovr, IOSAPIC,
    LSAPIC, Irq_src };

  struct Apic_head
  {
    Unsigned8 type;
    Unsigned8 len;
  } __attribute__((packed));

  struct Io_apic : public Apic_head
  {
    Unsigned8 id;
    Unsigned8 res;
    Unsigned32 adr;
    Unsigned32 irq_base;
  } __attribute__((packed));

  struct Irq_source : public Apic_head
  {
    Unsigned8  bus;
    Unsigned8  src;
    Unsigned32 irq;
    Unsigned16 flags;
  } __attribute__((packed));

public:
  Unsigned32 local_apic;
  Unsigned32 apic_flags;

private:
  char data[0];
} __attribute__((packed));


IMPLEMENTATION:

#include "kmem.h"
#include <cctype>

Acpi_rsdt const *Acpi::_rsdt;
Acpi_xsdt const *Acpi::_xsdt;
bool Acpi::_init_done;


static void
print_acpi_id(char const *id, unsigned len)
{
  char ID[len];
  for (unsigned i = 0; i < len; ++i)
    ID[i] = isalnum(id[i]) ? id[i] : '.';
  printf("%.*s", len, ID);
}

PUBLIC void
Acpi_rsdp::print_info() const
{
  printf("ACPI: RSDP[%p]\tr%02x OEM:", this, rev);
  print_acpi_id(oem, 6);
  printf("\n");
}

PUBLIC void
Acpi_table_head::print_info() const
{
  printf("ACPI: ");
  print_acpi_id(signature, 4);
  printf("[%p]\tr%02x OEM:", this, rev);
  print_acpi_id(oem_id, 6);
  printf(" OEMTID:");
  print_acpi_id(oem_tid, 8);
  printf("\n");
}

PUBLIC template< typename T >
void
Acpi_sdt<T>::print_summary() const
{
  for (unsigned i = 0; i < ((len-sizeof(Acpi_table_head))/sizeof(ptrs[0])); ++i)
    {
      Acpi_table_head const *t = Kmem::dev_map.map((Acpi_table_head const*)ptrs[i]);
      if (t == (Acpi_table_head const *)~0UL)
	continue;

      t->print_info();
    }
}


PUBLIC static
void
Acpi::init_virt()
{
  if (_init_done)
    return;
  _init_done = 1;

  printf("ACPI-Init\n");

  Acpi_rsdp const *rsdp = Acpi_rsdp::locate();
  if (!rsdp)
    {
      printf("ACPI: Could not find RSDP, skip init\n");
      return;
    }

  rsdp->print_info();

  if (rsdp->rev && rsdp->xsdt_phys)
    {
      Acpi_xsdt const *x = Kmem::dev_map.map((const Acpi_xsdt *)rsdp->xsdt_phys);
      if (x == (Acpi_xsdt const *)~0UL)
        printf("ACPI: Could not map XSDT\n");
      else if (!x->checksum_ok())
        printf("ACPI: Checksum mismatch in XSDT\n");
      else
        {
          _xsdt = x;
          x->print_info();
        }
    }

  if (rsdp->rsdt_phys)
    {
      Acpi_rsdt const *r = Kmem::dev_map.map((const Acpi_rsdt *)rsdp->rsdt_phys);
      if (r == (Acpi_rsdt const *)~0UL)
        printf("ACPI: Could not map RSDT\n");
      else if (!r->checksum_ok())
        printf("ACPI: Checksum mismatch in RSDT\n");
      else
        {
          _rsdt = r;
          r->print_info();
        }
    }

  if (_xsdt)
    _xsdt->print_summary();
  else if (_rsdt)
    _rsdt->print_summary();
}

PUBLIC static
template< typename T >
T
Acpi::find(const char *s)
{
  T a = 0;
  init_virt();
  if (_xsdt)
    a = static_cast<T>(_xsdt->find(s));
  else if (_rsdt)
    a = static_cast<T>(_rsdt->find(s));
  return a;
}

IMPLEMENT
Acpi_rsdt const *
Acpi_rsdp::rsdt() const
{
  return (Acpi_rsdt const*)rsdt_phys;
}

IMPLEMENT
Acpi_xsdt const *
Acpi_rsdp::xsdt() const
{
  if (rev == 0)
    return 0;
  return (Acpi_xsdt const*)xsdt_phys;
}

IMPLEMENT
bool
Acpi_rsdp::checksum_ok() const
{
  // ACPI 1.0 checksum
  Unsigned8 sum = 0;
  for (unsigned i = 0; i < 20; i++)
    sum += *((Unsigned8 *)this + i);

  if (sum)
    return false;

  if (rev == 0)
    return true;

  // Extended Checksum
  for (unsigned i = 0; i < len && i < 4096; ++i)
    sum += *((Unsigned8 *)this + i);

  return !sum;
}

IMPLEMENT
bool
Acpi_table_head::checksum_ok() const
{
  Unsigned8 sum = 0;
  for (unsigned i = 0; i < len && i < 4096; ++i)
    sum += *((Unsigned8 *)this + i);

  return !sum;
}

PUBLIC
template< typename T >
Acpi_table_head const *
Acpi_sdt<T>::find(char const sig[4]) const
{
  for (unsigned i = 0; i < ((len-sizeof(Acpi_table_head))/sizeof(ptrs[0])); ++i)
    {
      Acpi_table_head const *t = Kmem::dev_map.map((Acpi_table_head const*)ptrs[i]);
      if (t == (Acpi_table_head const *)~0UL)
	continue;

      if (t->signature[0] == sig[0]
	  && t->signature[1] == sig[1]
	  && t->signature[2] == sig[2]
	  && t->signature[3] == sig[3]
          && t->checksum_ok())
	return t;
    }

  return 0;
}

PUBLIC
Acpi_madt::Apic_head const *
Acpi_madt::find(Unsigned8 type, int idx) const
{
  for (unsigned i = 0; i < len-sizeof(Acpi_madt);)
    {
      Apic_head const *a = (Apic_head const *)(data + i);
      //printf("a=%p, a->type=%u, a->len=%u\n", a, a->type, a->len);
      if (a->type == type)
	{
	  if (!idx)
	    return a;
	  --idx;
	}
      i += a->len;
    }

  return 0;
}

// ------------------------------------------------------------------------
IMPLEMENTATION [ia32,amd64]:

IMPLEMENT
Acpi_rsdp const *
Acpi_rsdp::locate()
{
  enum
  {
    ACPI20_PC99_RSDP_START = 0x0e0000,
    ACPI20_PC99_RSDP_END =   0x100000
  };

  for (Address p = ACPI20_PC99_RSDP_START; p < ACPI20_PC99_RSDP_END; p += 16)
    {
      Acpi_rsdp const* r = (Acpi_rsdp const *)p;
      if (r->signature[0] == 'R' &&
	  r->signature[1] == 'S' &&
	  r->signature[2] == 'D' &&
	  r->signature[3] == ' ' &&
	  r->signature[4] == 'P' &&
	  r->signature[5] == 'T' &&
	  r->signature[6] == 'R' &&
	  r->signature[7] == ' ' &&
          r->checksum_ok())
	return r;
    }

  return 0;
}
