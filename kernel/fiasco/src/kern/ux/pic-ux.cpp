/*
 * Fiasco-UX
 * Architecture specific interrupt code
 */

INTERFACE:

#include "config.h"
#include <sys/poll.h>

EXTENSION class Pic
{
public:
  static int irq_pending();
  static void eat(unsigned);
  static void set_owner(int);
  static unsigned int map_irq_to_gate(unsigned);
  static bool setup_irq_prov(unsigned irq, const char * const path,
                             void (*bootstrap_func)());
  static void irq_prov_shutdown();
  static unsigned int get_pid_for_irq_prov(unsigned);
  static bool get_ipi_gate(unsigned irq, unsigned long &gate);

  enum
  {
    Irq_timer    = 0,
    Irq_con      = 1,
    Irq_net      = 2,
    Num_dev_irqs = 4,
    Irq_ipi_base = 4,
    Num_irqs     = 4 + Config::Max_num_cpus,
  };

private:
  static unsigned int highest_irq;
  static unsigned int pids[Pic::Num_irqs];
  static struct pollfd pfd[Pic::Num_irqs];
};

// ------------------------------------------------------------------------
INTERFACE[ux && mp]:

EXTENSION class Pic
{
public:
  static void send_ipi(Cpu_number _cpu, unsigned char data);
  static bool setup_ipi(Cpu_number _cpu, int _tid);

private:
  typedef Per_cpu_array<unsigned> Fd_array;
  static Fd_array ipi_fds;
};

// ------------------------------------------------------------------------
IMPLEMENTATION[ux]:

#include <cassert>			// for assert
#include <csignal>			// for SIGIO
#include <cstdio>			// for stdin
#include <cstdlib>			// for EXIT_FAILURE
#include <fcntl.h>			// for fcntl
#include <pty.h>			// for termios
#include <unistd.h>			// for fork
#include <sys/types.h>			// for fork

#include "boot_info.h"			// for boot_info::fd()
#include "emulation.h"
#include "initcalls.h"

unsigned int Pic::highest_irq;
unsigned int Pic::pids[Num_irqs];
struct pollfd Pic::pfd[Num_irqs];

PUBLIC static inline
unsigned
Pic::nr_irqs()
{ return Num_dev_irqs; }

IMPLEMENT FIASCO_INIT
void
Pic::init()
{
  atexit (&irq_prov_shutdown);
}

PRIVATE static
bool
Pic::prepare_irq(int sockets[2])
{
  struct termios tt;

  if (openpty(&sockets[0], &sockets[1], NULL, NULL, NULL))
    {
      perror("openpty");
      return false;
    }

  if (tcgetattr(sockets[0], &tt) < 0)
    {
      perror("tcgetattr");
      return false;
    }

  cfmakeraw(&tt);

  if (tcsetattr(sockets[0], TCSADRAIN, &tt) < 0)
    {
      perror("tcsetattr");
      return false;
    }

  fflush(NULL);

  return true;
}

IMPLEMENT
bool
Pic::setup_irq_prov(unsigned irq, const char * const path,
                    void (*bootstrap_func)())
{
  int sockets[2];

  if (access(path, X_OK | F_OK))
    {
      perror(path);
      return false;
    }

  if (prepare_irq(sockets) == false)
    return false;

  switch (pids[irq] = fork())
    {
      case -1:
        return false;

      case 0:
        break;

      default:
        close(sockets[1]);
        fcntl(sockets[0], F_SETFD, FD_CLOEXEC);
        pfd[irq].fd = sockets[0];
        return true;
    }

  // Unblock all signals except SIGINT, we enter jdb with it and don't want
  // the irq providers to die
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigprocmask(SIG_SETMASK, &mask, NULL);

  fclose(stdin);
  fclose(stdout);
  fclose(stderr);

  dup2(sockets[1], 0);
  close(sockets[0]);
  close(sockets[1]);
  bootstrap_func();

  _exit(EXIT_FAILURE);
}

IMPLEMENT
void
Pic::irq_prov_shutdown()
{
  for (unsigned int i = 0; i < Num_irqs; i++)
    if (pids[i])
      kill(pids[i], SIGTERM);
}

IMPLEMENT inline NEEDS [<cassert>, <csignal>, <fcntl.h>, "boot_info.h"]
void
Pic::enable_locked(unsigned irq, unsigned /*prio*/)
{
  int flags;

  // If fd is zero, someone tried to activate an IRQ without provider
  assert (pfd[irq].fd);

  if ((flags = fcntl(pfd[irq].fd, F_GETFL)) < 0				||
       fcntl(pfd[irq].fd, F_SETFL, flags | O_NONBLOCK | O_ASYNC) < 0	||
       fcntl(pfd[irq].fd, F_SETSIG, SIGIO) < 0				||
       fcntl(pfd[irq].fd, F_SETOWN, Boot_info::pid()) < 0)
    return;

  pfd[irq].events = POLLIN;

  if (irq >= highest_irq)
    highest_irq = irq + 1;
}

IMPLEMENT inline
void
Pic::disable_locked(unsigned)
{}

IMPLEMENT inline
void
Pic::acknowledge_locked(unsigned)
{}

IMPLEMENT inline
void
Pic::block_locked(unsigned)
{}

IMPLEMENT
int
Pic::irq_pending()
{
  unsigned int i;

  for (i = 0; i < highest_irq; i++)
    pfd[i].revents = 0;

  if (poll(pfd, highest_irq, 0) > 0)
    for (i = 0; i < highest_irq; i++)
      if (pfd[i].revents & POLLIN)
        {
	  if (!Emulation::idt_vector_present(0x20 + i))
            eat(i);
          else
            return i;
	}

  return -1;
}

IMPLEMENT inline NEEDS [<cassert>, <unistd.h>]
void
Pic::eat(unsigned irq)
{
  char buffer[8];

  assert (pfd[irq].events & POLLIN);

  while (read(pfd[irq].fd, buffer, sizeof (buffer)) > 0)
    ;
}

/*
 * Possible problem if an IRQ gets enabled and the system is already
 * long running and the owner is set wrong?
 */
IMPLEMENT inline
void
Pic::set_owner(int pid)
{
  for (unsigned int i = 0; i < highest_irq; i++)
    if (pfd[i].events & POLLIN)
      fcntl(pfd[i].fd, F_SETOWN, pid);
}

IMPLEMENT inline
unsigned int
Pic::map_irq_to_gate(unsigned irq)
{
  return 0x20 + irq;
}

IMPLEMENT
unsigned int
Pic::get_pid_for_irq_prov(unsigned irq)
{
  return pids[irq];
}

// ---------------------------------------------------------------------
IMPLEMENTATION[ux && !mp]:

IMPLEMENT static
bool
Pic::get_ipi_gate(unsigned, unsigned long &)
{ return false; }


// ---------------------------------------------------------------------
IMPLEMENTATION[ux && mp]:

#include "ipi.h"

Pic::Fd_array Pic::ipi_fds;

IMPLEMENT
void
Pic::set_cpu(unsigned irq_num, Cpu_number cpu)
{
  printf("Pic::set_cpu(%d, %d)\n", irq_num, cpu);
}

IMPLEMENT static
bool
Pic::setup_ipi(Cpu_number _cpu, int _tid)
{
  int sockets[2], flags;

  if (prepare_irq(sockets) == false)
    return false;

  unsigned i = Irq_ipi_base + _cpu;

  pfd[i].fd = sockets[0];
  ipi_fds[_cpu] = sockets[1];

  if ((flags = fcntl(pfd[i].fd, F_GETFL)) < 0			  ||
      fcntl(pfd[i].fd, F_SETFL, flags | O_NONBLOCK | O_ASYNC) < 0 ||
      fcntl(pfd[i].fd, F_SETSIG, SIGIO) < 0                       ||
      fcntl(pfd[i].fd, F_SETOWN, _tid) < 0)
    return false;

  pfd[i].events = POLLIN;

  if (i >= highest_irq)
    highest_irq = i + 1;

  return true;
}

IMPLEMENT
void
Pic::send_ipi(Cpu_number _cpu, unsigned char data)
{
  if (ipi_fds[_cpu])
    if (write(ipi_fds[_cpu], &data, sizeof(data)) != sizeof(data))
      printf("Write error\n");
}

IMPLEMENT static
bool
Pic::get_ipi_gate(unsigned irq, unsigned long &gate)
{
  if (irq < Irq_ipi_base)
    return false;

  // XXX check if irq is the irq for our current context/cpu
  unsigned char b;
  if (read(pfd[irq].fd, &b, sizeof(b)) < 1)
    {
      printf("read failure\n");
      return false;
    }

  printf("IPI message: %c\n", b);
  gate = Ipi::gate(b);

  return true;
}
