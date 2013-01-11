INTERFACE:

#include "task.h"

class Sigma0_task : public Task
{
public:
  explicit Sigma0_task(Ram_quota *q) : Task(q) {}
  bool is_sigma0() const { return true; }
  Address virt_to_phys_s0(void *virt) const { return (Address)virt; }
};


IMPLEMENTATION:

PUBLIC
bool
Sigma0_task::v_fabricate(Mem_space::Vaddr address,
                         Mem_space::Phys_addr* phys, Mem_space::Size* size,
                         unsigned* attribs = 0)
{
  // special-cased because we don't do ptab lookup for sigma0
  *size = Mem_space::has_superpages()
        ? Mem_space::Size(Config::SUPERPAGE_SIZE)
        : Mem_space::Size(Config::PAGE_SIZE);
  *phys = address.trunc(*size);

  if (attribs)
    *attribs = Mem_space::Page_writable
             | Mem_space::Page_user_accessible
             | Mem_space::Page_cacheable;

  return true;
}

PUBLIC inline virtual
Page_number
Sigma0_task::map_max_address() const
{ return Page_number::create(1UL << (MWORD_BITS - Mem_space::Page_shift)); }

