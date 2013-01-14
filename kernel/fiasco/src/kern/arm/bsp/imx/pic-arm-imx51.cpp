INTERFACE [arm && pic_gic && (imx51 || imx53 || imx6)]:

#include "gic.h"

INTERFACE [arm && pic_gic && (imx51 | imx53)]:

EXTENSION class Pic
{
  enum { Gic_sz = 7 };
};

INTERFACE [arm && pic_gic && imx6]:

EXTENSION class Pic
{
  enum { Gic_sz = 8 };
};

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && pic_gic && (imx51 || imx53 || imx6)]:

#include "irq_mgr_multi_chip.h"
#include "kmem.h"

IMPLEMENT FIASCO_INIT
void
Pic::init()
{
  typedef Irq_mgr_multi_chip<Gic_sz> M;

  M *m = new Boot_object<M>(1);

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

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && pic_gic && mp && imx6]:

PUBLIC static
void Pic::init_ap(unsigned)
{
  gic->init_ap();
}
