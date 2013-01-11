IMPLEMENTATION [io]:

PUBLIC
bool
Sigma0_task::v_fabricate(Io_space::Addr address, Io_space::Phys_addr* phys,
                         Io_space::Size* size, unsigned* attribs = 0)
{
  // special-cased because we don't do lookup for sigma0
  *phys = address.trunc(Io_space::Size(Io_space::Map_superpage_size));
  *size = Io_space::Size(Io_space::Map_superpage_size);
  if (attribs)
    *attribs = Io_space::Page_writable
             | Io_space::Page_user_accessible;
  return true;
}
