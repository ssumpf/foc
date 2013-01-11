IMPLEMENTATION:

#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdlib>

using namespace std;

#include "map_util.h"
#include "space.h"
#include "globals.h"
#include "config.h"

#include "boot_info.h"
#include "cpu.h"
#include "config.h"
#include "kip_init.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "per_cpu_data_alloc.h"
#include "pic.h"
#include "static_init.h"
#include "usermode.h"
#include "vmem_alloc.h"
#include "mem_space_sigma0.h"

IMPLEMENTATION:

typedef L4_fpage Test_fpage;

class Test_space : public Space
{
public:
  Test_space (Ram_quota *rq)
    : Space (Space::Default_factory(), rq, L4_fpage::mem(0x1200000, Config::PAGE_SHIFT))
  {}

  void* operator new (size_t s)
  { return malloc (s); }

  void operator delete (void *p)
  { ::free (p); }
};

class Timeout;

DEFINE_PER_CPU Per_cpu<Timeout *> timeslice_timeout;
STATIC_INITIALIZER_P(init, STARTUP_INIT_PRIO);
STATIC_INITIALIZER_P(init2, POST_CPU_LOCAL_INIT_PRIO);

static void init()
{
  Usermode::init(0);
  Boot_info::init();
  Kip_init::setup_ux();
  Kmem_alloc::base_init();
  Kip_init::init();
  Kmem_alloc::init();

  // Initialize cpu-local data management and run constructors for CPU 0
  Per_cpu_data::init_ctors();
  Per_cpu_data_alloc::alloc(0);
  Per_cpu_data::run_ctors(0);

}

static void init2()
{
  Cpu::init_global_features();
  Config::init();
  Kmem::init_mmu(*Cpu::boot_cpu());
  Vmem_alloc::init();
  Pic::init();
}

static Ram_quota rq;

struct Sigma0_space_factory
{
  static void create(Mem_space *v, Ram_quota *q)
  { new (v) Mem_space_sigma0(q); }

  template< typename S >
  static void create(S *v)
  { new (v) S(); }

};

int main()
{
  // 
  // Create tasks
  // 
  Space *sigma0 = new Space (Sigma0_space_factory(), &rq, L4_fpage::mem(0x1200000, Config::PAGE_SHIFT));

  init_mapdb_mem(sigma0);

  Test_space *server = new Test_space (&rq);
  assert (server);
  Test_space *client = new Test_space (&rq);
  assert (client);
  Test_space *client2 = new Test_space (&rq);
  assert (client2);

  // 
  // Manipulate mappings.
  // 
  Mapdb* mapdb = mapdb_instance();

  Mem_space::Phys_addr phys;
  Virt_size size;
  unsigned page_attribs;
  Mapping *m;
  Mapdb::Frame frame;
  
  // 
  // s0 [0x10000] -> server [0x1000]
  // 
  assert (server->mem_space()->v_lookup (Virt_addr(0x1000), &phys, &size, &page_attribs) 
	  == false);
  Reap_list reap;
  fpage_map (sigma0, 
	     Test_fpage::mem(0x10000, Config::PAGE_SHIFT, L4_fpage::RWX),
	     server,
	     L4_fpage::all_spaces(),
	     0x1000, &reap);

  assert (server->mem_space()->v_lookup (Virt_addr(0x1000), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::PAGE_SIZE));
  assert (phys == Virt_addr(0x10000));
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible));

  assert (mapdb->lookup (sigma0, Virt_addr(0x10000), Virt_addr(0x10000), &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);

  // 
  // s0 [0/superpage] -> server [0] -> should map many 4K pages and
  // overmap previous mapping
  // 
  assert (server->mem_space()->v_lookup (Virt_addr(0), &phys, &size, &page_attribs)
	  == false);

  fpage_map (sigma0, 
	     L4_fpage::mem(0, Config::SUPERPAGE_SHIFT, L4_fpage::RX),
	     server,
	     L4_fpage::all_spaces(),
	     0, &reap);

  assert (server->mem_space()->v_lookup (Virt_addr(0), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::PAGE_SIZE));	// and not SUPERPAGE_SIZE!
  assert (phys == Virt_addr(0));
  assert (page_attribs == Mem_space::Page_user_accessible);

  assert (mapdb->lookup (sigma0, Virt_addr(0), Virt_addr(0), &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);

  // previous mapping still there?

  assert (server->mem_space()->v_lookup (Virt_addr(0x1000), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::PAGE_SIZE));
  // Previously, overmapping did not work and was ignored, so the
  // lookup yielded the previously established mapping:
  //   assert (phys == 0x10000);
  //   assert (page_attribs == (Mem_space::Page_writable | Mem_space::Page_user_accessible));
  // Now, the previous mapping should have been overwritten:
  assert (phys == Virt_addr(0x1000));
  assert (page_attribs == Mem_space::Page_user_accessible);

  // mapdb entry -- tree should now contain another mapping 
  // s0 [0x10000] -> server [0x10000]
  assert (mapdb->lookup (sigma0, Virt_addr(0x10000), Virt_addr(0x10000), &m, &frame));
  print_node (m, frame, 0x10000, 0x11000);
  mapdb->free (frame);

  // 
  // Partially unmap superpage s0 [0/superpage]
  // 
  assert (server->mem_space()->v_lookup (Virt_addr(0x101000), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::PAGE_SIZE));
  assert (phys == Virt_addr(0x101000));
  assert (page_attribs == Mem_space::Page_user_accessible);

  fpage_unmap (sigma0, 
	       Test_fpage::mem(0x100000, Config::SUPERPAGE_SHIFT - 2, L4_fpage::RWX),
	       L4_map_mask(0) /*full unmap, not me too)*/, reap.list());
  
  assert (mapdb->lookup (sigma0, Virt_addr(0x0), Virt_addr(0x0), &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);
  
  assert (! server->mem_space()->v_lookup (Virt_addr(0x101000), &phys, &size, 
					   &page_attribs)
	  == true);

  // 
  // s0 [4M/superpage] -> server [8M]
  // 
  assert (server->mem_space()->v_lookup (Virt_addr(0x800000), &phys, &size, &page_attribs)
	  == false);

  fpage_map (sigma0,
	     Test_fpage::mem(0x400000, Config::SUPERPAGE_SHIFT, L4_fpage::RWX),
	     server,
	     L4_fpage::mem(0x800000, Config::SUPERPAGE_SHIFT),
	     0, &reap);

  assert (server->mem_space()->v_lookup (Virt_addr(0x800000), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::SUPERPAGE_SIZE));
  assert (phys == Virt_addr(0x400000));
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible));

  assert (mapdb->lookup (sigma0, Virt_addr(0x400000), Virt_addr(0x400000), &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);

  // 
  // server [8M+4K] -> client [8K]
  // 
  assert (client->mem_space()->v_lookup (Virt_addr(0x8000), &phys, &size, &page_attribs)
	  == false);

  fpage_map (server, 
	     Test_fpage::mem(0x801000, Config::PAGE_SHIFT, L4_fpage::RWX),
	     client,
	     L4_fpage::mem(0, L4_fpage::Whole_space),
	     0x8000, &reap);

  assert (client->mem_space()->v_lookup (Virt_addr(0x8000), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::PAGE_SIZE));
  assert (phys == Virt_addr(0x401000));
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible));

  // Previously, the 4K submapping is attached to the Sigma0 parent.
  // Not any more.

  assert (mapdb->lookup (sigma0, Virt_addr(0x400000), Virt_addr(0x400000), &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);

  //
  // Overmap a read-only page.  The writable attribute should not be
  // flushed.
  //
  fpage_map (server,
	     Test_fpage::mem(0x801000, Config::PAGE_SHIFT, L4_fpage::RX),
	     client,
	     L4_fpage::mem(0, L4_fpage::Whole_space),
	     0x8000, &reap);

  assert (client->mem_space()->v_lookup (Virt_addr(0x8000), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::PAGE_SIZE));
  assert (phys == Virt_addr(0x401000));
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible));  

#if 0 // no selective unmap anymore
  // 
  // Try selective unmap
  //
  fpage_map (server,
	     Test_fpage::mem(0x801000, Config::PAGE_SHIFT, L4_fpage::RWX),
	     client2,
	     L4_fpage::all_spaces(),
	     0x1000, &reap);

  assert (client2->mem_space()->v_lookup (0x1000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x401000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible));  

  assert (mapdb->lookup (sigma0, 0x400000, 0x400000, &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);

  fpage_unmap (server,
	       Test_fpage (false, true, Config::PAGE_SHIFT, 0x801000),
	       mask(false, client2->id(), true, false, false));

  // Page should have vanished in client2's space, but should still be
  // present in client's space.
  assert (client2->mem_space()->v_lookup (0x1000, &phys, &size, &page_attribs)
	  == false);
  assert (client->mem_space()->v_lookup (0x8000, &phys, &size, &page_attribs)
	  == true);
  assert (size == Config::PAGE_SIZE);
  assert (phys == 0x401000);
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible));  

  assert (mapdb->lookup (sigma0->id(), 0x400000, 0x400000, &m, &frame));
  print_node (m, frame);
  mapdb->free (frame);
#endif
  cerr << "... ";


  // 
  // Try some Accessed / Dirty flag unmaps
  // 
  
  // touch page in client
  assert (client->mem_space()->v_insert (Virt_addr(0x401000), Virt_addr(0x8000), Virt_size(Config::PAGE_SIZE), 
			    Mem_space::Page_dirty | Mem_space::Page_referenced)
	  == Mem_space::Insert_warn_attrib_upgrade);

  assert (client->mem_space()->v_lookup (Virt_addr(0x8000), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::PAGE_SIZE));
  assert (phys == Virt_addr(0x401000));
  assert (page_attribs == (Mem_space::Page_writable
			   | Mem_space::Page_user_accessible
			   | Mem_space::Page_dirty | Mem_space::Page_referenced));
  // reset dirty from server
  assert (fpage_unmap (server,
		       Test_fpage::mem(0x801000, Config::PAGE_SHIFT),
		       L4_map_mask(0), reap.list()
		       /*no_unmap + reset_refs*/)
	  == Mem_space::Page_referenced | Mem_space::Page_dirty);

  assert (client->mem_space()->v_lookup (Virt_addr(0x8000), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::PAGE_SIZE));
  assert (phys == Virt_addr(0x401000));
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible)); // Page_dirty | Page_referecned is gone...

  assert (server->mem_space()->v_lookup (Virt_addr(0x801000), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::SUPERPAGE_SIZE));
  assert (phys == Virt_addr(0x400000));
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible
			   | Mem_space::Page_dirty | Mem_space::Page_referenced)); // ...and went here

  // flush dirty and accessed from server
  assert (fpage_unmap (server, 
		       Test_fpage::mem(0x800000, Config::SUPERPAGE_SHIFT),
		       L4_map_mask(0x80000000), reap.list())
	  == Mem_space::Page_dirty | Mem_space::Page_referenced);

  assert (client->mem_space()->v_lookup (Virt_addr(0x8000), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::PAGE_SIZE));
  assert (phys == Virt_addr(0x401000));
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible)); // dirty/ref gone

  assert (server->mem_space()->v_lookup (Virt_addr(0x800000), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::SUPERPAGE_SIZE));
  assert (phys == Virt_addr(0x400000));
  assert (page_attribs == (Mem_space::Page_writable 
			   | Mem_space::Page_user_accessible)); // dirty/ref gone

  assert (sigma0->mem_space()->v_lookup (Virt_addr(0x400000), &phys, &size, &page_attribs)
	  == true);
  assert (size == Virt_size(Config::SUPERPAGE_SIZE));
  assert (phys == Virt_addr(0x400000));
  // Be a bit more lax in checking Sigma0's address space:  It does
  // not contain Page_writable / Page_user_accessible flags unless
  // they are faulted in.
  assert (page_attribs & (Mem_space::Page_dirty | Mem_space::Page_referenced));


  // 
  // Delete tasks
  // 
#if 0
  // do not do this because the mapping database would crash if 
  // they has mappings without spaces
  delete server;
  delete client;
  delete client2;
  delete sigma0;
#endif

  cerr << "OK" << endl;

  return 0;
}

static void print_node(Mapping* node, const Mapdb::Frame& frame,
		       Address va_begin = 0, Address va_end = ~0UL)
{
  assert (node);

  size_t size = Mapdb::shift(frame, node);

  for (Mapdb::Iterator i (frame, node, Virt_addr(va_begin), Virt_addr(va_end)); node;)
    {
      for (int d = node->depth(); d != 0; d--)
        cout << ' ';

      cout << setbase(16)
	   << "space=0x"  << (unsigned long) (node->space())
	   << " vaddr=0x" << node->page() << size
	   << " size=0x" << size;

      if (Mapping* p = node->parent())
	{
	  cout << " parent=0x" << (unsigned long)p->space()
	       << " p.vaddr=0x" << p->page().value() << size;
	}

      cout << endl;

      node = i;
      if (node)
	{
	  size = i.shift();
	  ++i;
	}
    }
  cout << endl;
}

