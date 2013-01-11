INTERFACE [arm && pic_gic && imx51]:

#include "gic.h"

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && pic_gic && imx51]:

#include "irq_mgr_multi_chip.h"
#include "kmem.h"

IMPLEMENT FIASCO_INIT
void
Pic::init()
{
  typedef Irq_mgr_multi_chip<7> M;

  M *m = new Boot_object<M>(1);

  gic.construct(0, Kmem::Gic_dist_map_base);
  m->add_chip(0, gic, gic->nr_irqs());

  Irq_mgr::mgr = m;
}

IMPLEMENT inline
Pic::Status Pic::disable_all_save()
{ return 0; }

IMPLEMENT inline
void Pic::restore_all(Status)
{}
