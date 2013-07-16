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

class Fake_factory : public Ram_quota
{
};

class Fake_task : public Space
{
public:
  Fake_task(Ram_quota *r, char const *name)
  : Space(r, Caps::all()), name(name) {}

  char const *name;
};


static void init_spaces()
{
  static Fake_factory rq;
#define NEW_TASK(name) name = new Fake_task(&rq, #name)
  NEW_TASK(s0);
  NEW_TASK(other);
  NEW_TASK(client);
  NEW_TASK(father);
  NEW_TASK(son);
  NEW_TASK(daughter);
  NEW_TASK(aunt);
#undef NEW_TASK
}

static
std::ostream &operator << (std::ostream &s, Mapdb::Pfn v)
{
  s << cxx::int_value<Mapdb::Pfn>(v);
  return s;
}

static
std::ostream &operator << (std::ostream &s, Mapping::Page v)
{
  s << cxx::int_value<Mapping::Page>(v);
  return s;
}

static
std::ostream &operator << (std::ostream &s, Space const &sp)
{
  Fake_task const *t = static_cast<Fake_task const *>(&sp);
  if (!t)
    s << "<NULL>";
  else
    s << t->name;
  return s;
}

static void print_node(Mapping* node, const Mapdb::Frame& frame)
{
  assert (node);

  for (Mapdb::Iterator i(frame, node, Mapdb::Pfn(0), Mapdb::Pfn(~0)); node;)
    {
      cout << "[UTEST] ";
      for (int d = node->depth(); d != 0; d--)
        cout << ' ';

      auto shift = i.order();

      cout << setbase(16)
	   << "space="  << *node->space()
	   << " vaddr=0x" << (node->page() << shift)
	   << " size=0x" << (Mapdb::Pfn(1) << shift);

      if (Mapping* p = node->parent())
	{
	  cout << " parent=" << *p->space()
	       << " p.vaddr=0x" << (p->page() << shift);
	}

      cout << endl;

      node = i;
      if (node)
        ++i;
    }
  cout << "[UTEST] " << endl;
}

static
Mapdb::Pfn to_pfn(Address a)
{ return Mem_space::to_pfn(Mem_space::V_pfn(Virt_addr(a))); }

static
Mapdb::Pcnt to_pcnt(unsigned order)
{ return Mem_space::to_pcnt(Mem_space::Page_order(order)); }

void basic()
{
  unsigned phys_bits = 32;
  Mapdb m (s0, Mapping::Page(1U << (phys_bits - Config::PAGE_SHIFT - page_sizes[0])), page_sizes, page_sizes_max);

  Mapping *node, *sub;
  Mapdb::Frame frame;

  typedef Mem_space SPACE;
  typedef SPACE::V_pfn V_pfn;
  typedef SPACE::V_pfc V_pfc;
  typedef SPACE::Phys_addr Phys_addr;
  typedef SPACE::Page_order Page_order;

  assert (! m.lookup(other, to_pfn(Config::PAGE_SIZE),
                     to_pfn(Config::PAGE_SIZE),
                     &node, &frame));

  cout << "[UTEST] Looking up 4M node at physaddr=0K" << endl;
  assert (m.lookup (s0,  to_pfn(0),
                    to_pfn(0),
                    &node, &frame));
  print_node (node, frame);

  cout << "[UTEST] Inserting submapping" << endl;
  sub = m.insert (frame, node, other,
		  to_pfn(2*Config::PAGE_SIZE),
		  to_pfn(Config::PAGE_SIZE),
		  to_pcnt(Config::PAGE_SHIFT));
  print_node (node, frame);

  m.free (frame);

  //////////////////////////////////////////////////////////////////////

  cout << "[UTEST] Looking up 4M node at physaddr=8M" << endl;
  assert (m.lookup (s0,
		    to_pfn(2*Config::SUPERPAGE_SIZE),
		    to_pfn(2*Config::SUPERPAGE_SIZE),
		    &node, &frame));
  print_node (node, frame);

  // XXX broken mapdb: assert (node->size() == Config::SUPERPAGE_SIZE);

  cout << "[UTEST] Inserting submapping" << endl;
  sub = m.insert (frame, node, other,
		  to_pfn(4*Config::SUPERPAGE_SIZE),
		  to_pfn(2*Config::SUPERPAGE_SIZE),
		  to_pcnt(Config::SUPERPAGE_SHIFT));
  print_node (node, frame);

  assert (m.shift(frame, sub) == Mapdb::Order(Config::SUPERPAGE_SHIFT - Config::PAGE_SHIFT));

  // Before we can insert new mappings, we must free the tree.
  m.free (frame);

  cout << "[UTEST] Get that mapping again" << endl;
  assert (m.lookup (other,
		    to_pfn(4*Config::SUPERPAGE_SIZE),
		    to_pfn(2*Config::SUPERPAGE_SIZE),
		    &sub, &frame));
  print_node (sub, frame);

  node = sub->parent();

  cout << "[UTEST] Inserting 4K submapping" << endl;
  assert ( m.insert (frame, sub, client,
		     to_pfn(15*Config::PAGE_SIZE),
		     to_pfn(2*Config::SUPERPAGE_SIZE),
		     to_pcnt(Config::PAGE_SHIFT)));
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
  Mapdb m (s0, Mapping::Page(1U << (32 - Config::PAGE_SHIFT - page_sizes[0])),
      page_sizes, page_sizes_max);

  Mapping *gf_map, *f_map, *son_map, *daughter_map;
  Mapdb::Frame frame;

  cout << "[UTEST] Looking up 4K node at physaddr=0" << endl;
  assert (m.lookup (grandfather, to_pfn(0),
		    to_pfn(0), &gf_map, &frame));
  print_whole_tree (gf_map, frame);

  cout << "[UTEST] Inserting father mapping" << endl;
  f_map = m.insert (frame, gf_map, father, to_pfn(0),
		    to_pfn(0),
		    to_pcnt(Config::PAGE_SHIFT));
  print_whole_tree (gf_map, frame);
  m.free(frame);


  cout << "[UTEST] Looking up father at physaddr=0" << endl;
  assert (m.lookup (father, to_pfn(0),
		    to_pfn(0), &f_map, &frame));
  print_whole_tree (f_map, frame);

  cout << "[UTEST] Inserting son mapping" << endl;
  son_map = m.insert (frame, f_map, son, to_pfn(0),
		      to_pfn(0),
		      to_pcnt(Config::PAGE_SHIFT));
  print_whole_tree (f_map, frame);
  m.free(frame);


  cout << "[UTEST] Looking up father at physaddr=0" << endl;
  assert (m.lookup (father, to_pfn(0),
		    to_pfn(0), &f_map, &frame));
  print_whole_tree (f_map, frame);

  cout << "[UTEST] Inserting daughter mapping" << endl;
  daughter_map = m.insert (frame, f_map, daughter, to_pfn(0),
			   to_pfn(0),
			   to_pcnt(Config::PAGE_SHIFT));
  print_whole_tree (f_map, frame);
  m.free(frame);


  cout << "[UTEST] Looking up son at physaddr=0" << endl;
  assert (m.lookup(son, to_pfn(0),
		   to_pfn(0), &son_map, &frame));
  f_map = son_map->parent();
  print_whole_tree (son_map, frame);

  cout << "[UTEST] Son has accident on return from disco" << endl;
  m.flush(frame, son_map, L4_map_mask::full(),
	  to_pfn(0),
	  to_pfn(Config::PAGE_SIZE));
  m.free(frame);

  cout << "[UTEST] Lost aunt returns from holidays" << endl;
  assert (m.lookup (grandfather, to_pfn(0),
		    to_pfn(0), &gf_map, &frame));
  print_whole_tree (gf_map, frame);

  cout << "[UTEST] Inserting aunt mapping" << endl;
  assert (m.insert (frame, gf_map, aunt, to_pfn(0),
		    to_pfn(0),
		    to_pcnt(Config::PAGE_SHIFT)));
  print_whole_tree (gf_map, frame);
  m.free(frame);

  cout << "[UTEST] Looking up daughter at physaddr=0" << endl;
  assert (m.lookup(daughter, to_pfn(0),
		   to_pfn(0), &daughter_map, &frame));
  print_whole_tree (daughter_map, frame);
  f_map = daughter_map->parent();
  cout << "[UTEST] Father of daugther is " << *f_map->space() << endl;

  assert(f_map->space() == father);

  m.free(frame);
}


void flushtest()
{
  Mapdb m (s0, Mapping::Page(1U << (32 - Config::PAGE_SHIFT - page_sizes[0])),
      page_sizes, page_sizes_max);

  Mapping *gf_map, *f_map;
  Mapdb::Frame frame;

  cout << "[UTEST] Looking up 4K node at physaddr=0" << endl;
  assert (m.lookup (grandfather, to_pfn(0), to_pfn(0), &gf_map, &frame));
  print_whole_tree (gf_map, frame);

  cout << "[UTEST] Inserting father mapping" << endl;
  f_map = m.insert (frame, gf_map, father, to_pfn(0), to_pfn(0), to_pcnt(Config::PAGE_SHIFT));
  print_whole_tree (gf_map, frame);
  m.free(frame);


  cout << "[UTEST] Looking up father at physaddr=0" << endl;
  assert (m.lookup (father, to_pfn(0), to_pfn(0), &f_map, &frame));
  print_whole_tree (f_map, frame);

  cout << "[UTEST] Inserting son mapping" << endl;
  assert (m.insert (frame, f_map, son, to_pfn(0), to_pfn(0), to_pcnt(Config::PAGE_SHIFT)));
  print_whole_tree (f_map, frame);
  m.free(frame);

  cout << "[UTEST] Lost aunt returns from holidays" << endl;
  assert (m.lookup (grandfather, to_pfn(0), to_pfn(0), &gf_map, &frame));
  print_whole_tree (gf_map, frame);

  cout << "[UTEST] Inserting aunt mapping" << endl;
  assert (m.insert (frame, gf_map, aunt, to_pfn(0), to_pfn(0), to_pcnt(Config::PAGE_SHIFT)));
  print_whole_tree (gf_map, frame);
  m.free(frame);

  cout << "[UTEST] Looking up father at physaddr=0" << endl;
  assert (m.lookup(father, to_pfn(0), to_pfn(0), &f_map, &frame));
  gf_map = f_map->parent();
  print_whole_tree (gf_map, frame);

  cout << "[UTEST] father is killed by his new love" << endl;
  m.flush(frame, f_map, L4_map_mask::full(), to_pfn(0), to_pfn(Config::PAGE_SIZE));
  print_whole_tree (gf_map, frame);
  m.free(frame);

  cout << "[UTEST] Try resurrecting the killed father again" << endl;
  assert (! m.lookup(father, to_pfn(0), to_pfn(0), &f_map, &frame));

  cout << "[UTEST] Resurrection is impossible, as it ought to be." << endl;
}

void multilevel ()
{
  size_t page_sizes[] = { 30 - Config::PAGE_SHIFT,
      22 - Config::PAGE_SHIFT,
      0};
  Mapdb m (s0, Mapping::Page(1U << (42 - Config::PAGE_SHIFT - page_sizes[0])),
      page_sizes, sizeof(page_sizes) / sizeof(page_sizes[0]));

  Mapping *node /* , *sub, *subsub */;
  Mapdb::Frame frame;

  cout << "[UTEST] Looking up 0xd2000000" << endl;
  assert (m.lookup (s0, to_pfn(0xd2000000), to_pfn(0xd2000000), &node, &frame));

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

class Timeout;

DEFINE_PER_CPU Per_cpu<Timeout *> timeslice_timeout;
STATIC_INITIALIZER_P(init, STARTUP_INIT_PRIO);
STATIC_INITIALIZER_P(init2, POST_CPU_LOCAL_INIT_PRIO);

static void init()
{
  Usermode::init(Cpu_number::boot_cpu());
  Boot_info::init();
  Kip_init::init();
  Kmem_alloc::base_init();
  Kmem_alloc::init();

  // Initialize cpu-local data management and run constructors for CPU 0
  Per_cpu_data::init_ctors();
  Per_cpu_data_alloc::alloc(Cpu_number::boot_cpu());
  Per_cpu_data::run_ctors(Cpu_number::boot_cpu());

}

static void init2()
{
  Cpu::init_global_features();
  Config::init();
  Kmem::init_mmu(Cpu::cpus.cpu(Cpu_number::boot_cpu()));
}

int main()
{
  init_spaces();
  cout << "[UTEST] Basic test" << endl;
  basic();
  cout << "[UTEST] ########################################" << endl;

  cout << "[UTEST] Hole test" << endl;
  maphole();
  cout << "[UTEST] ########################################" << endl;

  cout << "[UTEST] Flush test" << endl;
  flushtest();
  cout << "[UTEST] ########################################" << endl;

  cout << "Multilevel test" << endl;
  multilevel();
  cout << "[UTEST] ########################################" << endl;

  cerr << "OK" << endl;
  return(0);
}
