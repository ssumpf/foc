INTERFACE [arm]:

#include "kip.h"
#include "mem_layout.h"

class Kmem : public Mem_layout
{
public:

  static Mword is_kmem_page_fault( Mword pfa, Mword error );
  static Mword is_ipc_page_fault( Mword pfa, Mword error );
  static Mword is_io_bitmap_page_fault( Mword pfa );

  static Mword ipc_window( unsigned num );
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

IMPLEMENT inline
Mword Kmem::is_kmem_page_fault( Mword pfa, Mword /*error*/ )
{
  return in_kernel(pfa);
}

IMPLEMENT inline
Mword Kmem::is_io_bitmap_page_fault( Mword /*pfa*/ )
{
  return 0;
}
