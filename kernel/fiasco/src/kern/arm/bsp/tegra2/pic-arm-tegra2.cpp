INTERFACE [arm && pic_gic && tegra2]:

#include "gic.h"

//-------------------------------------------------------------------
IMPLEMENTATION [arm && pic_gic && tegra2]:

#include "irq_chip.h"
#include "irq_mgr_multi_chip.h"
#include "gic.h"
#include "kmem.h"

IMPLEMENT FIASCO_INIT
void Pic::init()
{
  typedef Irq_mgr_multi_chip<8> M;

  M *m = new Boot_object<M>(16);

  gic.construct(Kmem::Gic_cpu_map_base, Kmem::Gic_dist_map_base);
  m->add_chip(0, gic, gic->nr_irqs());

  Irq_mgr::mgr = m;
}

IMPLEMENT inline
Pic::Status Pic::disable_all_save()
{ return 0; }

IMPLEMENT inline
void Pic::restore_all(Status)
{}

//-------------------------------------------------------------------
IMPLEMENTATION [arm && mp && pic_gic && tegra2]:

PUBLIC static
void Pic::init_ap()
{
  gic->init_ap();
}
