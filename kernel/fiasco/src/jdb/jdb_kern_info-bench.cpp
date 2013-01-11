INTERFACE:

#include "jdb.h"

class Jdb_kern_info_bench : public Jdb_kern_info_module
{
private:
  static Unsigned64 get_time_now();
  static void show_arch();
};

//---------------------------------------------------------------------------
IMPLEMENTATION:

static Jdb_kern_info_bench k_a INIT_PRIORITY(JDB_MODULE_INIT_PRIO+1);

PUBLIC
Jdb_kern_info_bench::Jdb_kern_info_bench()
  : Jdb_kern_info_module('b', "Benchmark privileged instructions")
{
  Jdb_kern_info::register_subcmd(this);
}

PUBLIC
void
Jdb_kern_info_bench::show()
{
  do_mp_benchmark();
  show_arch();
}

//---------------------------------------------------------------------------
IMPLEMENTATION [!mp]:

PRIVATE
void
Jdb_kern_info_bench::do_mp_benchmark()
{}

//---------------------------------------------------------------------------
IMPLEMENTATION [mp && (ia32 || amd64)]:

#include "idt.h"

PRIVATE static inline
void
Jdb_kern_info_bench::stop_timer()
{
  Timer_tick::set_vectors_stop();
}

//---------------------------------------------------------------------------
IMPLEMENTATION [mp && !(ia32 || amd64)]:

PRIVATE static inline
void
Jdb_kern_info_bench::stop_timer()
{}

//---------------------------------------------------------------------------
IMPLEMENTATION [mp]:

#include "ipi.h"

static int volatile ipi_bench_spin_done;
static int ipi_cnt;

PRIVATE static
void
Jdb_kern_info_bench::wait_for_ipi(unsigned cpu, void *)
{
  Jdb::restore_irqs(cpu);
  stop_timer();
  Proc::sti();

  while (!ipi_bench_spin_done)
    Proc::pause();

  Proc::cli();
  Jdb::save_disable_irqs(cpu);
}

PRIVATE static
void
Jdb_kern_info_bench::empty_func(unsigned, void *)
{
  ++ipi_cnt;
}

PRIVATE static
void
Jdb_kern_info_bench::do_ipi_bench(unsigned my_cpu, void *_partner)
{
  Unsigned64 time;
  unsigned partner = (unsigned long)_partner;
  enum {
    Runs2  = 3,
    Warmup = 4,
    Rounds = (1 << Runs2) + Warmup,
  };
  unsigned i;

  ipi_cnt = 0;
  Mem::barrier();

  for (i = 0; i < Warmup; ++i)
    Jdb::remote_work_ipi(my_cpu, partner, empty_func, 0, true);

  time = get_time_now();
  for (i = 0; i < (1 << Runs2); i++)
    Jdb::remote_work_ipi(my_cpu, partner, empty_func, 0, true);

  printf(" %2u:%8lld", partner, (get_time_now() - time) >> Runs2);

  if (ipi_cnt != Rounds)
    printf("\nCounter mismatch: cnt=%d v %d\n", ipi_cnt, Rounds);

  ipi_bench_spin_done = 1;
  Mem::barrier();
}

PRIVATE
void
Jdb_kern_info_bench::do_mp_benchmark()
{
  // IPI bench matrix
  printf("IPI round-trips:\n");
  for (unsigned u = 0; u < Config::Max_num_cpus; ++u)
    if (Cpu::online(u))
      {
        printf("l%2u(p%8u): ", u, Cpu::cpus.cpu(u).phys_id());

	for (unsigned v = 0; v < Config::Max_num_cpus; ++v)
	  if (Cpu::online(v))
	    {
	      if (u == v)
		printf(" %2u:%8s", u, "X");
	      else
		{
		  ipi_bench_spin_done = 0;

		  // v is waiting for IPIs
		  if (v != 0)
		    Jdb::remote_work(v, wait_for_ipi, 0, false);

		  // u is doing benchmark
		  if (u == 0)
		    do_ipi_bench(0, (void *)v);
		  else
                    Jdb::remote_work(u, do_ipi_bench, (void *)v, false);

		  // v is waiting for IPIs
		  if (v == 0)
		    wait_for_ipi(0, 0);

		  Mem::barrier();

		  while (!ipi_bench_spin_done)
		    Proc::pause();
		}
	    }
	printf("\n");
      }
}
