IMPLEMENTATION:

#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdlib>

using namespace std;

#include "mapdb.h"

IMPLEMENTATION:

#include "config.h"
#include "space.h"

static Space *s0;
static Space *other;
static Space *client;
static Space *father;
static Space *&grandfather = s0;
static Space *son;
static Space *daughter;
static Space *aunt;

typedef Virt_addr Phys_addr;

static size_t page_sizes[] =
{ Config::SUPERPAGE_SHIFT - Config::PAGE_SHIFT, 0 };

static size_t page_sizes_max = 2;


static void init_spaces()
{
  static Ram_quota rq;
  L4_fpage utcb_area = L4_fpage::mem(0x1200000, Config::PAGE_SHIFT);
  s0       = new Space(Space::Default_factory(), &rq, utcb_area);
  other    = new Space(Space::Default_factory(), &rq, utcb_area);
  client   = new Space(Space::Default_factory(), &rq, utcb_area);
  father   = new Space(Space::Default_factory(), &rq, utcb_area);
  son      = new Space(Space::Default_factory(), &rq, utcb_area);
  daughter = new Space(Space::Default_factory(), &rq, utcb_area);
  aunt     = new Space(Space::Default_factory(), &rq, utcb_area);
}

static void print_node(Mapping* node, const Mapdb::Frame& frame)
{
  assert (node);

  size_t shift;

  for (Mapdb::Iterator i(frame, node, Virt_addr(0), Virt_addr(~0)); node;)
    {
      for (int d = node->depth(); d != 0; d--)
        cout << ' ';

      shift = i.shift();

      cout << setbase(16)
	   << "space=0x"  << (unsigned) (node->space())
	   << " vaddr=0x" << (node->page() << shift).value()
	   << " size=0x" << (1UL << shift);

      if (Mapping* p = node->parent())
	{
	  cout << " parent=0x" << p->space()
	       << " p.vaddr=0x" << (p->page() << shift).value();
	}

      cout << endl;

      node = i;
      if (node)
	{
	  shift = i.shift();
	  ++i;
	}
    }
  cout << endl;
}

void basic()
{
  Mapdb m (s0, Page_number(1U << (32 - Config::SUPERPAGE_SHIFT)), page_sizes, page_sizes_max);

  Mapping *node, *sub, *subsub;
  Mapdb::Frame frame;

  assert (! m.lookup(other, Mem_space::Addr::create(Config::PAGE_SIZE),
		     Mem_space::Phys_addr::create(Config::PAGE_SIZE),
		     &node, &frame));

  cout << "Looking up 4M node at physaddr=0K" << endl;
  assert (m.lookup (s0,  Mem_space::Addr::create(0),
		    Mem_space::Phys_addr::create(0),
		    &node, &frame));
  print_node (node, frame);

  cout << "Inserting submapping" << endl;
  sub = m.insert (frame, node, other,
		  Mem_space::Addr::create(2*Config::PAGE_SIZE),
		  Mem_space::Phys_addr::create(Config::PAGE_SIZE),
		  Mem_space::Size::create(Config::PAGE_SIZE));
  print_node (node, frame);

  m.free (frame);

  //////////////////////////////////////////////////////////////////////

  cout << "Looking up 4M node at physaddr=8M" << endl;
  assert (m.lookup (s0,
		    Mem_space::Addr::create(2*Config::SUPERPAGE_SIZE),
		    Mem_space::Phys_addr::create(2*Config::SUPERPAGE_SIZE),
		    &node, &frame));
  print_node (node, frame);

  // XXX broken mapdb: assert (node->size() == Config::SUPERPAGE_SIZE);

  cout << "Inserting submapping" << endl;
  sub = m.insert (frame, node, other,
		  Mem_space::Addr::create(4*Config::SUPERPAGE_SIZE),
		  Mem_space::Phys_addr::create(2*Config::SUPERPAGE_SIZE),
		  Mem_space::Size::create(Config::SUPERPAGE_SIZE));
  print_node (node, frame);

  assert (m.shift(frame, sub) == Config::SUPERPAGE_SHIFT- Config::PAGE_SHIFT);

  // Before we can insert new mappings, we must free the tree.
  m.free (frame);

  cout << "Get that mapping again" << endl;
  assert (m.lookup (other,
		    Mem_space::Addr::create(4*Config::SUPERPAGE_SIZE),
		    Mem_space::Phys_addr::create(2*Config::SUPERPAGE_SIZE),
		    &sub, &frame));
  print_node (sub, frame);

  node = sub->parent();

  cout << "Inserting 4K submapping" << endl;
  subsub = m.insert (frame, sub, client,
		     Mem_space::Addr::create(15*Config::PAGE_SIZE),
		     Mem_space::Phys_addr::create(2*Config::SUPERPAGE_SIZE),
		     Mem_space::Size::create(Config::PAGE_SIZE));
  print_node (node, frame);

  m.free (frame);
}

static void print_whole_tree(Mapping *node, const Mapdb::Frame& frame)
{
  while(node->parent())
    node = node->parent();
  print_node (node, frame);
}


void maphole()
{
  Mapdb m(s0, Page_number(1U << (32 - Config::SUPERPAGE_SHIFT)),
      page_sizes, page_sizes_max);

  Mapping *gf_map, *f_map, *son_map, *daughter_map, *a_map;
  Mapdb::Frame frame;

  cout << "Looking up 4K node at physaddr=0" << endl;
  assert (m.lookup (grandfather, Mem_space::Addr::create(0),
		    Mem_space::Phys_addr::create(0), &gf_map, &frame));
  print_whole_tree (gf_map, frame);

  cout << "Inserting father mapping" << endl;
  f_map = m.insert (frame, gf_map, father, Mem_space::Addr::create(0),
		    Mem_space::Phys_addr::create(0),
		    Mem_space::Size::create(Config::PAGE_SIZE));
  print_whole_tree (gf_map, frame);
  m.free(frame);


  cout << "Looking up father at physaddr=0" << endl;
  assert (m.lookup (father, Mem_space::Addr::create(0),
		    Mem_space::Phys_addr::create(0), &f_map, &frame));
  print_whole_tree (f_map, frame);

  cout << "Inserting son mapping" << endl;
  son_map = m.insert (frame, f_map, son, Mem_space::Addr::create(0),
		      Mem_space::Phys_addr::create(0),
		      Mem_space::Size::create(Config::PAGE_SIZE));
  print_whole_tree (f_map, frame);
  m.free(frame);


  cout << "Looking up father at physaddr=0" << endl;
  assert (m.lookup (father, Mem_space::Addr::create(0),
		    Mem_space::Phys_addr::create(0), &f_map, &frame));
  print_whole_tree (f_map, frame);

  cout << "Inserting daughter mapping" << endl;
  daughter_map = m.insert (frame, f_map, daughter, Mem_space::Addr::create(0),
			   Mem_space::Phys_addr::create(0),
			   Mem_space::Size::create(Config::PAGE_SIZE));
  print_whole_tree (f_map, frame);
  m.free(frame);


  cout << "Looking up son at physaddr=0" << endl;
  assert (m.lookup(son, Mem_space::Addr::create(0),
		   Mem_space::Phys_addr::create(0), &son_map, &frame));
  f_map = son_map->parent();
  print_whole_tree (son_map, frame);

  cout << "Son has accident on return from disco" << endl;
  m.flush(frame, son_map, L4_map_mask::full(),
	  Mem_space::Addr::create(0),
	  Mem_space::Addr::create(Config::PAGE_SIZE));
  m.free(frame);

  cout << "Lost aunt returns from holidays" << endl;
  assert (m.lookup (grandfather, Mem_space::Addr::create(0),
		    Mem_space::Phys_addr::create(0), &gf_map, &frame));
  print_whole_tree (gf_map, frame);

  cout << "Inserting aunt mapping" << endl;
  a_map = m.insert (frame, gf_map, aunt, Mem_space::Addr::create(0),
		    Mem_space::Phys_addr::create(0),
		    Mem_space::Size::create(Config::PAGE_SIZE));
  print_whole_tree (gf_map, frame);
  m.free(frame);

  cout << "Looking up daughter at physaddr=0" << endl;
  assert (m.lookup(daughter, Mem_space::Addr::create(0),
		   Mem_space::Phys_addr::create(0), &daughter_map, &frame));
  print_whole_tree (daughter_map, frame);
  f_map = daughter_map->parent();
  cout << "Father of daugther is " << (unsigned)(f_map->space()) << endl;

  assert(f_map->space() == father);

  m.free(frame);
}


void flushtest()
{
  Mapdb m(s0, Page_number(1U << (32 - Config::SUPERPAGE_SHIFT)), page_sizes, page_sizes_max);

  Mapping *gf_map, *f_map, *son_map, *a_map;
  Mapdb::Frame frame;

  cout << "Looking up 4K node at physaddr=0" << endl;
  assert (m.lookup (grandfather, Virt_addr(0), Phys_addr(0), &gf_map, &frame));
  print_whole_tree (gf_map, frame);

  cout << "Inserting father mapping" << endl;
  f_map = m.insert (frame, gf_map, father, Virt_addr(0), Phys_addr(0), Virt_size(Config::PAGE_SIZE));
  print_whole_tree (gf_map, frame);
  m.free(frame);


  cout << "Looking up father at physaddr=0" << endl;
  assert (m.lookup (father, Virt_addr(0), Phys_addr(0), &f_map, &frame));
  print_whole_tree (f_map, frame);

  cout << "Inserting son mapping" << endl;
  son_map = m.insert (frame, f_map, son, Virt_addr(0), Phys_addr(0), Virt_size(Config::PAGE_SIZE));
  print_whole_tree (f_map, frame);
  m.free(frame);

  cout << "Lost aunt returns from holidays" << endl;
  assert (m.lookup (grandfather, Virt_addr(0), Phys_addr(0), &gf_map, &frame));
  print_whole_tree (gf_map, frame);

  cout << "Inserting aunt mapping" << endl;
  a_map = m.insert (frame, gf_map, aunt, Virt_addr(0), Phys_addr(0), Virt_size(Config::PAGE_SIZE));
  print_whole_tree (gf_map, frame);
  m.free(frame);

  cout << "Looking up father at physaddr=0" << endl;
  assert (m.lookup(father, Virt_addr(0), Phys_addr(0), &f_map, &frame));
  gf_map = f_map->parent();
  print_whole_tree (gf_map, frame);

  cout << "father is killed by his new love" << endl;
  m.flush(frame, f_map, L4_map_mask::full(), Virt_addr(0), Virt_addr(Config::PAGE_SIZE));
  print_whole_tree (gf_map, frame);
  m.free(frame);

  cout << "Try resurrecting the killed father again" << endl;
  assert (! m.lookup(father, Virt_addr(0), Phys_addr(0), &f_map, &frame));

  cout << "Resurrection is impossible, as it ought to be." << endl;
}

void multilevel ()
{
  size_t three_page_sizes[] = { 30 - Config::PAGE_SHIFT,
      22 - Config::PAGE_SHIFT,
      0};
  Mapdb m (s0, Page_number(4), three_page_sizes, 3);

  Mapping *node /* , *sub, *subsub */;
  Mapdb::Frame frame;

  cout << "Looking up 0xd2000000" << endl;
  assert (m.lookup (s0, Virt_addr(0xd2000000), Phys_addr(0xd2000000), &node, &frame));

  print_node (node, frame);
}

#include "boot_info.h"
#include "cpu.h"
#include "config.h"
#include "kip_init.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "per_cpu_data_alloc.h"
#include "static_init.h"
#include "usermode.h"
#include "vmem_alloc.h"

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
  Kmem::init_mmu(Cpu::cpus.cpu(0));
  printf("ha\n");
  Vmem_alloc::init();
}

int main()
{
  init_spaces();
  cout << "Basic test" << endl;
  basic();
  cout << "########################################" << endl;

  cout << "Hole test" << endl;
  maphole();
  cout << "########################################" << endl;

  cout << "Flush test" << endl;
  flushtest();
  cout << "########################################" << endl;

  cout << "Multilevel test" << endl;
  multilevel();
  cout << "########################################" << endl;

  cerr << "OK" << endl;
  return(0);
}
