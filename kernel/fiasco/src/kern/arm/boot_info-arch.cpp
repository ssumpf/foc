/* ARM specific boot_info */

INTERFACE [arm]:

class Kip;

EXTENSION class Boot_info
{
public:
  static void set_kip(Kip *kip);
  static Kip *kip();
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cstring>
#include <cstdio> // for debug printf's

static Kip *boot_info_kip;

IMPLEMENT
void Boot_info::set_kip(Kip *kip)
{
  boot_info_kip = kip;
}

IMPLEMENT
Kip *Boot_info::kip()
{
  return boot_info_kip;
}

extern "C" char _etext, _sstack, _stack, _edata, _end;

IMPLEMENT static
void Boot_info::init()
{
  // We save the checksum for read-only data to be able to compare it against
  // the kernel image later (in jdb::enter_kdebug())
  //saved_checksum_ro = boot_info::get_checksum_ro();
}

PUBLIC static
void
Boot_info::reset_checksum_ro(void)
{}
