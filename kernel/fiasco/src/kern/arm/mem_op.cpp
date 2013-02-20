INTERFACE [arm]:

#include "types.h"

class Mem_op
{
public:
  enum Op_cache
  {
    Op_cache_clean_data        = 0x00,
    Op_cache_flush_data        = 0x01,
    Op_cache_inv_data          = 0x02,
    Op_cache_coherent          = 0x03,
    Op_cache_dma_coherent      = 0x04,
    Op_cache_dma_coherent_full = 0x05,
    Op_cache_l2_clean          = 0x06,
    Op_cache_l2_flush          = 0x07,
    Op_cache_l2_inv            = 0x08,
  };

  enum Op_mem
  {
    Op_mem_read_data     = 0x10,
    Op_mem_write_data    = 0x11,
  };
};

// ------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "context.h"
#include "entry_frame.h"
#include "globals.h"
#include "mem.h"
#include "mem_space.h"
#include "mem_unit.h"
#include "outer_cache.h"
#include "space.h"
#include "warn.h"

PRIVATE static void
Mem_op::l1_inv_dcache(Address start, Address end)
{
  if (start & Mem_unit::Cache_line_mask)
    {
      Mem_unit::flush_dcache((void *)start, (void *)start);
      start += Mem_unit::Cache_line_size;
      start &= ~Mem_unit::Cache_line_mask;
    }
  if (end & Mem_unit::Cache_line_mask)
    {
      Mem_unit::flush_dcache((void *)end, (void *)end);
      end &= ~Mem_unit::Cache_line_mask;
    }

  if (start < end)
    Mem_unit::inv_dcache((void *)start, (void *)end);
}

PRIVATE static void
Mem_op::inv_icache(Address start, Address end)
{
  if (Address(end) - Address(start) > 0x2000)
    asm volatile("mcr p15, 0, r0, c7, c5, 0");
  else
    {
      for (start &= ~Mem_unit::Icache_line_mask;
           start < end; start += Mem_unit::Icache_line_size)
	asm volatile("mcr p15, 0, %0, c7, c5, 1" : : "r" (start));
    }
}

PUBLIC static void
Mem_op::arm_mem_cache_maint(int op, void const *start, void const *end)
{
  Context *c = current();

  if (EXPECT_FALSE(start > end))
    return;

  c->set_ignore_mem_op_in_progress(true);

  switch (op)
    {
    case Op_cache_clean_data:
      Mem_unit::clean_dcache(start, end);
      break;

    case Op_cache_flush_data:
      Mem_unit::flush_dcache(start, end);
      break;

    case Op_cache_inv_data:
      l1_inv_dcache((Address)start, (Address)end);
      break;

    case Op_cache_coherent:
      Mem_unit::clean_dcache(start, end);
      Mem::dsb();
      Mem_unit::btc_inv();
      inv_icache(Address(start), Address(end));
      Mem::dsb();
      break;

    case Op_cache_l2_clean:
    case Op_cache_l2_flush:
    case Op_cache_l2_inv:
      outer_cache_op(op, Address(start), Address(end));
      break;

    case Op_cache_dma_coherent:
        {
          Mem_unit::flush_dcache(Virt_addr(Address(start)), Virt_addr(Address(end)));
          outer_cache_op(Op_cache_l2_flush, Address(start), Address(end));
        }
      break;

    // We might not want to implement this one but single address outer
    // cache flushing can be really slow
    case Op_cache_dma_coherent_full:
      Mem_unit::flush_dcache();
      Outer_cache::flush();
      break;

    default:
      break;
    };

  c->set_ignore_mem_op_in_progress(false);
}

PUBLIC static void
Mem_op::arm_mem_access(Mword *r)
{
  Address  a = r[1];
  unsigned w = r[2];

  if (w > 2)
    return;

  if (!current()->space()->is_user_memory(a, 1 << w))
    return;

  jmp_buf pf_recovery;
  int e;

  if ((e = setjmp(pf_recovery)) == 0)
    {
      current()->recover_jmp_buf(&pf_recovery);

      switch (r[0])
	{
	case Op_mem_read_data:
	  switch (w)
	    {
	    case 0:
	      r[3] = *(unsigned char *)a;
	      break;
	    case 1:
	      r[3] = *(unsigned short *)a;
	      break;
	    case 2:
	      r[3] = *(unsigned int *)a;
	      break;
	    default:
	      break;
	    };
	  break;

	case Op_mem_write_data:
	  switch (w)
	    {
	    case 0:
	      *(unsigned char *)a = r[3];
	      break;
	    case 1:
	      *(unsigned short *)a = r[3];
	      break;
	    case 2:
	      *(unsigned int *)a = r[3];
	      break;
	    default:
	      break;
	    };
	  break;

	default:
	  break;
	};
    }
  else
    WARN("Unresolved memory access, skipping\n");

  current()->recover_jmp_buf(0);
}

extern "C" void sys_arm_mem_op()
{
  Entry_frame *e = current()->regs();
  if (EXPECT_FALSE(e->r[0] & 0x10))
    Mem_op::arm_mem_access(e->r);
  else
    Mem_op::arm_mem_cache_maint(e->r[0], (void *)e->r[1], (void *)e->r[2]);
}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && !outer_cache]:

PRIVATE static inline
void
Mem_op::outer_cache_op(int, Address, Address)
{}

// ------------------------------------------------------------------------
IMPLEMENTATION [arm && outer_cache]:

PRIVATE static
void
Mem_op::outer_cache_op(int op, Address start, Address end)
{

  Virt_addr s = Virt_addr(start);
  Virt_addr v = Virt_addr(start);
  Virt_addr e = Virt_addr(end);

  Context *c = current();

  while (v < e)
    {
      Mem_space::Size phys_size;
      Mem_space::Phys_addr phys_addr;
      unsigned attrs;
      bool mapped =    c->mem_space()->v_lookup(Mem_space::Vaddr(v), &phys_addr, &phys_size, &attrs)
                    && (attrs & Mem_space::Page_user_accessible);

      Virt_size sz = Virt_size(phys_size);
      Virt_size offs = Virt_size(Virt_addr(v).value() & (Mem_space::Size(phys_size).value() - 1));
      sz -= offs;
      if (e - v < sz)
        sz = e - v;

      if (mapped)
        {
          Virt_addr vstart = Virt_addr(phys_addr) | offs;
          Virt_addr vend = vstart + sz;
          switch (op)
            {
            case Op_cache_l2_clean:
              Outer_cache::clean(Virt_addr(vstart).value(),
                                 Virt_addr(vend).value(), false);
              break;
            case Op_cache_l2_flush:
              Outer_cache::flush(Virt_addr(vstart).value(),
                                 Virt_addr(vend).value(), false);
              break;
            case Op_cache_l2_inv:
              Outer_cache::invalidate(Virt_addr(vstart).value(),
                                      Virt_addr(vend).value(), false);
              break;
            }
        }
      v += sz;
    }
  Outer_cache::sync();
}
