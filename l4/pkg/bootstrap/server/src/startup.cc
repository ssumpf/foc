/**
 * \file	bootstrap/server/src/startup.c
 * \brief	Main functions
 *
 * \date	09/2004
 * \author	Torsten Frenzel <frenzel@os.inf.tu-dresden.de>,
 *		Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *		Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *		Alexander Warg <aw11@os.inf.tu-dresden.de>
 *		Sebastian Sumpf <sumpf@os.inf.tu-dresden.de>
 */

/*
 * (c) 2005-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

/* LibC stuff */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

/* L4 stuff */
#include <l4/sys/compiler.h>
#include <l4/util/mb_info.h>
#include <l4/util/l4_macros.h>
#include "panic.h"

/* local stuff */
#include "exec.h"
#include "macros.h"
#include "region.h"
#include "module.h"
#include "startup.h"
#include "support.h"
#include "init_kip.h"
#include "patch.h"
#include "loader_mbi.h"
#include "startup.h"
#include "koptions.h"

#if defined (ARCH_ppc32)
#include <l4/drivers/of_if.h>
#include <l4/drivers/of_dev.h>
#endif

#undef getchar

enum { Verbose_load = 0 };

static l4util_mb_info_t *mb_info;
/* management of allocated memory regions */
static Region_list regions;
static Region __regs[300];

/* management of conventional memory regions */
static Region_list ram;
static Region __ram[8];

#if defined(ARCH_x86) || defined(ARCH_amd64)
static l4util_mb_vbe_mode_t __mb_vbe;
static l4util_mb_vbe_ctrl_t __mb_ctrl;
#endif

L4_kernel_options::Uart kuart;
unsigned int kuart_flags;

/*
 * IMAGE_MODE means that all boot modules are linked together to one
 * big binary.
 */
#ifdef IMAGE_MODE
static l4_addr_t _mod_addr = RAM_BASE + MODADDR;
#else
static l4_addr_t _mod_addr;
#endif

static const char *builtin_cmdline = CMDLINE;

/* modules to load by bootstrap */
static int sigma0 = 1;   /* we need sigma0 */
static int roottask = 1; /* we need a roottask */

enum {
  kernel_module,
  sigma0_module,
  roottask_module,
};

/* we define a small stack for sigma0 and roottask here -- it is used by L4
 * for parameter passing. however, sigma0 and roottask must switch to its
 * own private stack as soon as it has initialized itself because this memory
 * region is later recycled in init.c */
static char roottask_init_stack[64]; /* XXX hardcoded */
static char sigma0_init_stack[64]; /* XXX hardcoded */


static exec_handler_func_t l4_exec_read_exec;
static exec_handler_func_t l4_exec_add_region;


#if 0
static void
dump_mbi(l4util_mb_info_t *mbi)
{
  printf("%p-%p\n", (void*)(mbi->mem_lower << 10), (void*)(mbi->mem_upper << 10));
  printf("MBI:     [%p-%p]\n", mbi, mbi + 1);
  printf("MODINFO: [%p-%p]\n", (char*)mbi->mods_addr,
      (l4util_mb_mod_t*)(mbi->mods_addr) + mbi->mods_count);

  printf("VBEINFO: [%p-%p]\n", (char*)mbi->vbe_ctrl_info,
      (l4util_mb_vbe_ctrl_t*)(mbi->vbe_ctrl_info) + 1);
  printf("VBEMODE: [%p-%p]\n", (char*)mbi->vbe_mode_info,
      (l4util_mb_vbe_mode_t*)(mbi->vbe_mode_info) + 1);

  l4util_mb_mod_t *m = (l4util_mb_mod_t*)(mbi->mods_addr);
  l4util_mb_mod_t *me = m + mbi->mods_count;
  for (; m < me; ++m)
    {
      printf("  MOD: [%p-%p]\n", (void*)m->mod_start, (void*)m->mod_end);
      printf("  CMD: [%p-%p]\n", (char*)m->cmdline,
	  (char*)m->cmdline + strlen((char*)m->cmdline));
    }
}
#endif


/**
 * Scan the memory regions with type == Region::Kernel for a
 * kernel interface page (KIP).
 *
 * After loading the kernel we scan for the magic number at page boundaries.
 */
static
void *find_kip()
{
  unsigned char *p, *end;
  void *k = 0;

  printf("  find kernel info page...\n");
  for (Region const *m = regions.begin(); m != regions.end(); ++m)
    {
      if (m->type() != Region::Kernel)
	continue;

      if (sizeof(unsigned long) < 8
          && m->end() >= (1ULL << 32))
	end = (unsigned char *)(~0UL - 0x1000);
      else
	end = (unsigned char *) (unsigned long)m->end();

      for (p = (unsigned char *) (unsigned long)(m->begin() & 0xfffff000);
	   p < end;
	   p += 0x1000)
	{
	  l4_umword_t magic = L4_KERNEL_INFO_MAGIC;
	  if (memcmp(p, &magic, 4) == 0)
	    {
	      k = p;
	      printf("  found kernel info page at %p\n", p);
	      break;
	    }
	}
    }

  if (!k)
    panic("could not find kernel info page, maybe your kernel is too old");

  return k;
}

static
L4_kernel_options::Options *find_kopts(void *kip)
{
  unsigned long a = (unsigned long)kip + sizeof(l4_kernel_info_t);

  // kernel-option directly follow the KIP page
  a = (a + 4096 - 1) & ~0xfff;

  L4_kernel_options::Options *ko = (L4_kernel_options::Options *)a;

  if (ko->magic != L4_kernel_options::Magic)
    panic("Could not find kernel options page");

  return ko;
}

const char *get_cmdline(l4util_mb_info_t *mbi)
{
  if (mbi && mbi->flags & L4UTIL_MB_CMDLINE)
    return L4_CHAR_PTR(mbi->cmdline);

  if (*builtin_cmdline)
    return builtin_cmdline;

  return 0;
}

/**
 * Get the API version from the KIP.
 */
static inline
unsigned long get_api_version(void *kip)
{
  return ((unsigned long *)kip)[1];
}


static char *
check_arg_str(char *cmdline, const char *arg)
{
  char *s = cmdline;
  while ((s = strstr(s, arg)))
    {
      if (s == cmdline
          || isspace(s[-1]))
        return s;
    }
  return NULL;
}

/**
 * Scan the command line for the given argument.
 *
 * The cmdline string may either be including the calling program
 * (.../bootstrap -arg1 -arg2) or without (-arg1 -arg2) in the realmode
 * case, there, we do not have a leading space
 *
 * return pointer after argument, NULL if not found
 */
char *
check_arg(l4util_mb_info_t *mbi, const char *arg)
{
  const char *c = get_cmdline(mbi);
  if (c)
    return check_arg_str((char *)c, arg);

  return NULL;
}


/**
 * Calculate the maximum memory limit in MB.
 *
 * The limit is the highes physical address where conventional RAM is allowed.
 *
 * If available the '-maxmem=xx' command line option is used.
 * If not then the memory is limited to 3 GB IA32 and unlimited on other
 * systems.
 */
static
unsigned long
get_memory_limit(l4util_mb_info_t *mbi)
{
  unsigned long arch_limit = ~0UL;
#if defined(ARCH_x86)
  /* Limit memory, we cannot really handle more right now. In fact, the
   * problem is roottask. It maps as many superpages/pages as it gets.
   * After that, the remaining pages are mapped using l4sigma0_map_anypage()
   * with a receive window of L4_WHOLE_ADDRESS_SPACE. In response Sigma0
   * could deliver pages beyond the 3GB user space limit. */
  arch_limit = 3024UL << 20;
#endif

  /* maxmem= parameter? */
  if (char *c = check_arg(mbi, "-maxmem="))
    {
      unsigned long l = strtoul(c + 8, NULL, 10) << 20;
      if (l < arch_limit)
	return l;
    }

  return arch_limit;
}

static int
parse_memvalue(const char *s, unsigned long *val, char **ep)
{

  *val = strtoul(s, ep, 0);
  if (*val == ~0UL)
    return 1;

  switch (**ep)
    {
    case 'G': *val <<= 10;
    case 'M': *val <<= 10;
    case 'k': case 'K': *val <<= 10; (*ep)++;
    };

  return 0;
}

/*
 * Parse a memory layout string: size@offset
 * E.g.: 256M@0x40000000, or 128M@128M
 */
static int
parse_mem_layout(const char *s, unsigned long *sz, unsigned long *offset)
{
  char *ep;

  if (parse_memvalue(s, sz, &ep))
    return 1;

  if (*sz == 0)
    return 1;

  if (*ep != '@')
    return 1;

  if (parse_memvalue(ep + 1, offset, &ep))
    return 1;

  return 0;
}

static void
dump_ram_map(bool show_total = false)
{
  // print RAM summary
  unsigned long long sum = 0;
  for (Region *i = ram.begin(); i < ram.end(); ++i)
    {
      printf("  RAM: %016llx - %016llx: %lldkB\n",
             i->begin(), i->end(), (i->end() - i->begin() + 1) >> 10);
      sum += i->end() - i->begin() + 1;
    }
  if (show_total)
    printf("  Total RAM: %lldMB\n", sum >> 20);
}

static void
setup_memory_map(l4util_mb_info_t *mbi)
{
  bool parsed_mem_option = false;
  const char *s = get_cmdline(mbi);

  if (s)
    {
      while ((s = check_arg_str((char *)s, "-mem=")))
        {
          s += 5;
          unsigned long sz, offset = 0;
          if (!parse_mem_layout(s, &sz, &offset))
            {
              parsed_mem_option = true;
              ram.add(Region::n(offset, offset + sz, ".ram", Region::Ram));
            }
        }
    }

  if (!parsed_mem_option)
    // No -mem option given, use the one given by the platform
    Platform_base::platform->setup_memory_map(mbi, &ram, &regions);

  dump_ram_map(true);
}

static void do_the_memset(unsigned long s, unsigned val, unsigned long len)
{
  printf("Presetting memory %16lx - %16lx to '%x'\n",
         s, s + len - 1, val);
  memset((void *)s, val, len);
}

static void fill_mem(unsigned fill_value)
{
  regions.sort();
  for (Region const *r = ram.begin(); r != ram.end(); ++r)
    {
      unsigned long long b = r->begin();
      // The regions list must be sorted!
      for (Region const *reg = regions.begin(); reg != regions.end(); ++reg)
        {
          // completely before ram?
          if (reg->end() < r->begin())
            continue;
          // completely after ram?
          if (reg->begin() > r->end())
            break;

          if (reg->begin() <= r->begin())
            b = reg->end() + 1;
          else if (b > reg->begin()) // some overlapping
            {
              if (reg->end() + 1 > b)
                b = reg->end() + 1;
            }
          else
            {
              do_the_memset(b, fill_value, reg->begin() - 1 - b + 1);
              b = reg->end() + 1;
            }
        }

      if (b < r->end())
        do_the_memset(b, fill_value, r->end() - b + 1);
    }
}

static void
move_module(l4util_mb_info_t *mbi, int i, Region *from, Region *to,
            bool overlap_check)
{
  unsigned long start = from->begin();
  unsigned long size = from->end() - start + 1;

  if (Verbose_load)
    {
      unsigned char c[5];
      c[0] = *(unsigned char*)(start + 0);
      c[1] = *(unsigned char*)(start + 1);
      c[2] = *(unsigned char*)(start + 2);
      c[3] = *(unsigned char*)(start + 3);
      c[4] = 0;
      c[0] = c[0] < 32 ? '.' : c[0];
      c[1] = c[1] < 32 ? '.' : c[1];
      c[2] = c[2] < 32 ? '.' : c[2];
      c[3] = c[3] < 32 ? '.' : c[3];
      printf("  moving module %02d { %lx, %llx } (%s) -> { %llx - %llx } [%ld]\n",
             i, start, from->end(), c, to->begin(), to->end(), size);

      for (int a = 0; a < 0x100; a += 4)
        printf("%08lx%s", *(unsigned long *)(start + a), (a % 32 == 28) ? "\n" : " ");
      printf("\n");
    }
  else
    printf("  moving module %02d { %lx-%llx } -> { %llx-%llx } [%ld]\n",
           i, start, from->end(), to->begin(), to->end(), size);

  if (!ram.contains(*to))
    panic("Panic: Would move outside of RAM");

  if (overlap_check)
    {
      Region *overlap = regions.find(*to);
      if (overlap)
        {
          printf("ERROR: module target [%llx-%llx) overlaps\n",
                 to->begin(), to->end());
          overlap->vprint();
          regions.dump();
          panic("cannot move module");
        }
    }
  memmove((void *)to->begin(), (void *)start, size);
  unsigned long x = to->end() + 1;
  memset((char *)x, 0, l4_round_page(x) - x);

  (L4_MB_MOD_PTR(mbi->mods_addr))[i].mod_start = to->begin();
  (L4_MB_MOD_PTR(mbi->mods_addr))[i].mod_end   = to->end() + 1;
  from->begin(to->begin());
  from->end(to->end());
}

static inline
unsigned long mbi_mod_start(l4util_mb_info_t *mbi, int i)
{ return (L4_MB_MOD_PTR(mbi->mods_addr))[i].mod_start; }

static inline
unsigned long mbi_mod_end(l4util_mb_info_t *mbi, int i)
{ return (L4_MB_MOD_PTR(mbi->mods_addr))[i].mod_end; }

static inline
unsigned long mbi_mod_size(l4util_mb_info_t *mbi, int i)
{ return mbi_mod_end(mbi, i) - mbi_mod_start(mbi, i); }

/**
 * Move modules to another address.
 *
 * Source and destination regions may overlap.
 */
static void
move_modules(l4util_mb_info_t *mbi, unsigned long modaddr)
{
  printf("  Moving up to %d modules behind %lx\n", mbi->mods_count, modaddr);

  Region *ramr = ram.find(Region(modaddr, modaddr));
  Region module_area(modaddr, ramr->end(), "ram for modules");

  unsigned long firstmodulestart = ~0UL, lastmoduleend = 0;
  for (unsigned i = 0; i < mbi->mods_count; ++i)
    {
      if (lastmoduleend < mbi_mod_end(mbi, i))
        lastmoduleend = mbi_mod_end(mbi, i);
      if (firstmodulestart > mbi_mod_start(mbi, i))
        firstmodulestart = mbi_mod_start(mbi, i);
    }
  lastmoduleend = l4_round_page(lastmoduleend);
  if (firstmodulestart < modaddr)
    {
      Region s(lastmoduleend, ramr->end());
      unsigned long sz = modaddr - firstmodulestart;
      lastmoduleend = regions.find_free(s, sz, L4_PAGESHIFT) + sz;
    }

  for (unsigned i = 0; i < mbi->mods_count; ++i)
    {
      unsigned long start = mbi_mod_start(mbi, i);
      unsigned long end = mbi_mod_end(mbi, i);
      unsigned long size = mbi_mod_size(mbi, i);

      if (start == end)
        continue;

      Region from(start, end - 1);
      Region *this_module = regions.find(from);
      assert(this_module->begin() == from.begin()
             && this_module->end() == from.end());

      if (i < 3)
        {
          if (start < lastmoduleend)
            {
              Region to(lastmoduleend, lastmoduleend + (end - start) - 1);
              if (module_area.contains(to))
                {
                  move_module(mbi, i, this_module, &to, true);
                  lastmoduleend = l4_round_page(this_module->end());
                }
            }
          continue;
        }

      if (start >= modaddr)
        continue;

      unsigned long long to = regions.find_free(module_area, size, L4_PAGESHIFT);
      assert(to);

      Region m_to = Region(to, to + size - 1);
      move_module(mbi, i, this_module, &m_to, true);
    }

  // now everything is behind modaddr -> pull close to modaddr now
  // this is optional but avoids holes and gives more consecutive memory

  if (0)
    printf("  Compactifying\n");

  regions.sort();
  unsigned long lastend = modaddr;
  for (Region *i = regions.begin(); i < regions.end(); ++i)
    {
      if (i->begin() < modaddr)
        continue;

      // find in mbi
      unsigned mi = 0;
      for (; mi < mbi->mods_count; ++mi)
        if (i->begin() == mbi_mod_start(mbi, mi))
          break;

      if (mi < 3 || mbi->mods_count == mi)
        continue;

      unsigned long start = mbi_mod_start(mbi, mi);
      unsigned long end = mbi_mod_end(mbi, mi);

      if (start > lastend)
        {
          Region to(lastend, end - 1 - (start - lastend));
          move_module(mbi, mi, i, &to, false);
          end = i->end();
        }
      lastend = l4_round_page(end);
    }
}


/**
 * Add the bootstrap binary itself to the allocated memory regions.
 */
static void
init_regions()
{
  extern int _start;	/* begin of image -- defined in crt0.S */
  extern int _end;	/* end   of image -- defined by bootstrap.ld */

  regions.add(Region::n((unsigned long)&_start, (unsigned long)&_end,
              ".bootstrap", Region::Boot));
}


/**
 * Add the memory containing the boot modules to the allocated regions.
 */
static void
add_boot_modules_region(l4util_mb_info_t *mbi)
{
  for (unsigned int i = 0; i < mbi->mods_count; ++i)
    regions.add(Region(mbi_mod_start(mbi, i), mbi_mod_end(mbi, i) - 1,
                       ".Module", Region::Root));
}


/**
 * Add all sections of the given ELF binary to the allocated regions.
 * Actually does not load the ELF binary (see load_elf_module()).
 */
static void
add_elf_regions(l4util_mb_info_t *mbi, l4_umword_t module,
                Region::Type type)
{
  exec_task_t exec_task;
  l4_addr_t entry;
  int r;
  const char *error_msg;
  l4util_mb_mod_t *mb_mod = (l4util_mb_mod_t*)mbi->mods_addr;

  assert(module < mbi->mods_count);

  exec_task.begin = 0xffffffff;
  exec_task.end   = 0;
  exec_task.type = type;

  exec_task.mod_start = L4_VOID_PTR(mb_mod[module].mod_start);
  exec_task.mod       = mb_mod + module;

  printf("  Scanning %s\n", L4_CHAR_PTR(mb_mod[module].cmdline));

  r = exec_load_elf(l4_exec_add_region, &exec_task,
                    &error_msg, &entry);

  if (r)
    {
      if (Verbose_load)
        {
          printf("\n%p: ", exec_task.mod_start);
          for (int i = 0; i < 4; ++i)
            {
              printf("%08lx ", *((unsigned long *)exec_task.mod_start + i));
            }
          printf("  ");
          for (int i = 0; i < 16; ++i)
            {
              unsigned char c = *(unsigned char *)((char *)exec_task.mod_start + i);
              printf("%c", c < 32 ? '.' : c);
            }
        }
      panic("\n\nThis is an invalid binary, fix it (%s).", error_msg);
    }
}


/**
 * Load the given ELF binary into memory and free the source
 * memory region.
 */
static l4_addr_t
load_elf_module(l4util_mb_mod_t *mb_mod)
{
  exec_task_t exec_task;
  l4_addr_t entry;
  int r;
  const char *error_msg;

  exec_task.begin = 0xffffffff;
  exec_task.end   = 0;

  exec_task.mod_start = L4_VOID_PTR(mb_mod->mod_start);
  exec_task.mod       = mb_mod;

  r = exec_load_elf(l4_exec_read_exec, &exec_task,
                    &error_msg, &entry);

#ifndef RELEASE_MODE
  /* clear the image for debugging and security reasons */
  memset(L4_VOID_PTR(mb_mod->mod_start), 0,
         mb_mod->mod_end - mb_mod->mod_start);
#endif

  if (r)
    printf("  => can't load module (%s)\n", error_msg);
  else
    {
      Region m = Region::n(mb_mod->mod_start, mb_mod->mod_end);
      Region *x = regions.find(m);
      if (x)
	{
	  if (x->begin() == m.begin())
	    {
	      unsigned long b = l4_round_page(m.end()+1);
	      if (x->end() <= b)
		regions.remove(x);
	      else
		x->begin(b);
	    }
	}
    }

  return entry;
}

/**
 * Simple linear memory allocator.
 *
 * Allocate size bytes startting from *ptr and set *ptr to *ptr + size.
 */
static inline void*
lin_alloc(l4_size_t size, char **ptr)
{
  void *ret = *ptr;
  *ptr += (size + 3) & ~3;;
  return ret;
}


/**
 * Duplicate the given command line.
 *
 * This function is use for relocating the multi-boot info.
 * The new location is *ptr and *ptr is incemented by the size of the
 * string (basically like lin_alloc() does).
 *
 * This function also implements the mechanism to replace the command line
 * of a module from the bootstrap comand line.
 */
static
char *dup_cmdline(l4util_mb_info_t *mbi, unsigned mod_nr, char **ptr,
    char const *orig)
{
  char *res = *ptr;
  if (!orig)
    return 0;

  char* name_end = strchr(orig, ' ');
  if (name_end && *name_end)
    *name_end = 0;
  else
    name_end = 0;

  unsigned size;
  char const *new_args = get_arg_module(mbi, orig, &size);

  if (new_args && size)
    printf("    new args for %d = \"%.*s\"\n", mod_nr, size, new_args);
  else
    if (name_end)
      *name_end = ' ';

  strcpy(*ptr, orig);
  *ptr+= strlen(*ptr)+1;

  if (new_args)
    {
      *((*ptr)-1) = ' ';
      strncpy(*ptr, new_args, size);
      *ptr += size;
      *((*ptr)++) = 0;
    }
  return res;
}


static
void
print_e820_map(l4util_mb_info_t *mbi)
{
#ifndef ARCH_arm
  printf("  Bootloader MMAP%s\n", mbi->flags & L4UTIL_MB_MEM_MAP
                                   ? ":" : " not available.");
#endif

  if (mbi->flags & L4UTIL_MB_MEM_MAP)
    {
      l4util_mb_addr_range_t *mmap;
      l4util_mb_for_each_mmap_entry(mmap, mbi)
	{
	  const char *types[] = { "unknown", "RAM", "reserved", "ACPI",
                                  "ACPI NVS", "unusable" };
	  const char *type_str = (mmap->type < (sizeof(types) / sizeof(types[0])))
                                 ? types[mmap->type] : types[0];

	  printf("    [%9llx, %9llx) %s (%d)\n",
                 (unsigned long long) mmap->addr,
                 (unsigned long long) mmap->addr + (unsigned long long) mmap->size,
                 type_str, (unsigned) mmap->type);
	}
    }
}

/**
 * Relocate and compact the multi-boot infomation (MBI).
 *
 * This function relocates the MBI into the first 4MB of physical memory.
 * Substructures such as module information, the VESA information, and
 * the command lines of the modules are also relocated.
 * During relocation of command lines they may be substituted according
 * to '-arg=' options from the bootstrap command line.
 *
 * The memory map is discared and not relocated, because everything after 
 * bootstrap has to use the KIP memory desriptors.
 */
static
l4util_mb_info_t *
relocate_mbi(l4util_mb_info_t *src_mbi, unsigned long* start,
             unsigned long* end)
{
  l4util_mb_info_t *dst_mbi;
  l4_addr_t x;

  print_e820_map(src_mbi);

  static char mbi_store[16 << 10] __attribute__((aligned(L4_PAGESIZE)));

  x = (l4_addr_t)&mbi_store;

  void *mbi_start = (void*)x;

  char *p = (char*)x;
  *start = x;

  dst_mbi = (l4util_mb_info_t*)lin_alloc(sizeof(l4util_mb_info_t), &p);

  /* copy (extended) multiboot info structure */
  memcpy(dst_mbi, src_mbi, sizeof(l4util_mb_info_t));

  dst_mbi->flags &= ~(L4UTIL_MB_CMDLINE | L4UTIL_MB_MEM_MAP | L4UTIL_MB_MEMORY);

  /* copy extended VIDEO information, if available */
  if (dst_mbi->flags & L4UTIL_MB_VIDEO_INFO)
    {
      if (src_mbi->vbe_mode_info)
	{
	  l4util_mb_vbe_mode_t *m
	    = (l4util_mb_vbe_mode_t*)lin_alloc(sizeof(l4util_mb_vbe_mode_t),
		&p);

	  memcpy(m, L4_VOID_PTR(src_mbi->vbe_mode_info),
		 sizeof(l4util_mb_vbe_mode_t));
	  dst_mbi->vbe_mode_info = (l4_addr_t)m;
	}

      /* copy VBE controller info structure */
      if (src_mbi->vbe_ctrl_info)
	{
	  l4util_mb_vbe_ctrl_t *m
	    = (l4util_mb_vbe_ctrl_t*)lin_alloc(sizeof(l4util_mb_vbe_ctrl_t),
		&p);
	  memcpy(m, L4_VOID_PTR(src_mbi->vbe_ctrl_info),
		 sizeof(l4util_mb_vbe_ctrl_t));
	  dst_mbi->vbe_ctrl_info = (l4_addr_t)m;
	}
    }

  /* copy module descriptions */
  l4util_mb_mod_t *mods = (l4util_mb_mod_t*)lin_alloc(sizeof(l4util_mb_mod_t)*
      src_mbi->mods_count, &p);
  memcpy(mods, L4_VOID_PTR(dst_mbi->mods_addr),
	 dst_mbi->mods_count * sizeof (l4util_mb_mod_t));
  dst_mbi->mods_addr = (l4_addr_t)mods;

  /* copy command lines of modules */
  for (unsigned i = 0; i < dst_mbi->mods_count; i++)
    {
      char *n = dup_cmdline(src_mbi, i, &p, (char const *)(mods[i].cmdline));
      if (n)
	  mods[i].cmdline = (l4_addr_t) n;
    }
  *end = (l4_addr_t)p;

  printf("  Relocated mbi to [%p-%p]\n", mbi_start, (void*)(*end));
  regions.add(Region::n((unsigned long)mbi_start,
                        ((unsigned long)*end) + 0xfe,
                        ".Multiboot info", Region::Root),
              true /* we overlap with the Bootstrap binary, we are in BSS*/);

  return dst_mbi;
}

#ifdef IMAGE_MODE

#ifdef COMPRESS
#include "uncompress.h"
#endif

//#define DO_CHECK_MD5
#ifdef DO_CHECK_MD5
#include <bsd/md5.h>

static void check_md5(const char *name, u_int8_t *start, unsigned size,
                      const char *md5sum)
{
  MD5_CTX md5ctx;
  unsigned char digest[MD5_DIGEST_LENGTH];
  char s[MD5_DIGEST_STRING_LENGTH];
  static const char hex[] = "0123456789abcdef";
  int j;

  printf(" Checking checksum of %s\n", name);

  MD5Init(&md5ctx);
  MD5Update(&md5ctx, start, size);
  MD5Final(digest, &md5ctx);

  for (j = 0; j < MD5_DIGEST_LENGTH; j++)
    {
      s[j + j] = hex[digest[j] >> 4];
      s[j + j + 1] = hex[digest[j] & 0x0f];
    }
  s[j + j] = '\0';

  if (strcmp(s, md5sum))
    panic("md5sum mismatch");
}
#else
static inline void check_md5(const char *, u_int8_t *, unsigned, const char *)
{}
#endif

typedef struct
{
  l4_uint32_t  start;
  l4_uint32_t  size;
  l4_uint32_t  size_uncompressed;
  l4_uint32_t  name;
  l4_uint32_t  md5sum_compr;
  l4_uint32_t  md5sum_uncompr;
} mod_info;

extern mod_info _module_info_start[];
extern mod_info _module_info_end[];

extern l4util_mb_mod_t _modules_mbi_start[];
extern l4util_mb_mod_t _modules_mbi_end[];

/**
 * Create the basic multi-boot structure in IMAGE_MODE
 */
static void
construct_mbi(l4util_mb_info_t *mbi)
{
  unsigned i;
  l4util_mb_mod_t *mods = _modules_mbi_start;
#ifdef COMPRESS
  l4_addr_t destbuf;
#endif

  mbi->mods_count  = _module_info_end - _module_info_start;
  mbi->flags      |= L4UTIL_MB_MODS;
  mbi->mods_addr   = (l4_addr_t)mods;

  assert(mbi->mods_count >= 2);

  for (i = 0; i < mbi->mods_count; ++i)
    check_md5(L4_CHAR_PTR(_module_info_start[i].name),
              (u_int8_t *)_module_info_start[i].start,
              _module_info_start[i].size,
              L4_CHAR_PTR(_module_info_start[i].md5sum_compr));

#ifdef COMPRESS
  printf("Compressed modules:\n");
  for (i = 0; i < mbi->mods_count; i++)
    {
      printf("  mod%02u: %08x-%08x: %s\n",
	     i, _module_info_start[i].start,
             _module_info_start[i].start + _module_info_start[i].size,
	     L4_CHAR_PTR(_module_info_start[i].name));
    }

  destbuf = l4_round_page(mods[mbi->mods_count - 1].mod_end);
  if (destbuf < _mod_addr)
    destbuf = _mod_addr;

  // advance to last module end
  for (i = 0; i < mbi->mods_count; i++)
    destbuf += l4_round_page(_module_info_start[i].size_uncompressed);

  // check for overlaps and adjust mod_addr accordingly
  unsigned long d = destbuf;
  for (i = mbi->mods_count; i > 0; --i)
    {
      d -= l4_round_page(_module_info_start[i - 1].size_uncompressed);
      if (d <= _module_info_start[i-1].start + _module_info_start[i-1].size)
        {
          l4_addr_t x = (_module_info_start[i-1].start + _module_info_start[i-1].size + 0xfff) & ~0xfff;
          l4_addr_t delta = x - d;

          _mod_addr += delta;
          destbuf += delta;
          printf("  Adjusting modaddr to %lx {%d}\n", _mod_addr, i-1);
          d = x;
        }
    }

  printf("Uncompressing modules (modaddr = %lx):\n", _mod_addr);
#endif

  for (i = mbi->mods_count; i > 0; --i)
    {
#ifdef COMPRESS
      destbuf -= l4_round_page(_module_info_start[i - 1].size_uncompressed);

      l4_addr_t image =
	 (l4_addr_t)decompress(L4_CONST_CHAR_PTR(_module_info_start[i - 1].name),
                               L4_VOID_PTR(_module_info_start[i - 1].start),
                               (void *)destbuf,
                               _module_info_start[i - 1].size,
                               _module_info_start[i - 1].size_uncompressed);
#else
      l4_addr_t image = _module_info_start[i - 1].start;
#endif
      mods[i - 1].mod_start = image;
      mods[i - 1].mod_end   = image + _module_info_start[i - 1].size_uncompressed;
      printf("  mod%02u: %08x-%08x: %s\n",
	     i - 1, mods[i - 1].mod_start, mods[i - 1].mod_end,
	     L4_CHAR_PTR(_module_info_start[i - 1].name));
      if (image == 0)
	panic("Panic: Failure decompressing image\n");
      if (!ram.contains(Region(mods[i - 1].mod_start,
                               mods[i - 1].mod_end)))
        panic("Panic: Module does not fit into RAM");

    }

  for (i = 0; i < mbi->mods_count; ++i)
    check_md5(L4_CHAR_PTR(_module_info_start[i].name),
              (u_int8_t *)mods[i].mod_start,
              _module_info_start[i].size_uncompressed,
              L4_CHAR_PTR(_module_info_start[i].md5sum_uncompr));
}
#endif /* IMAGE_MODE */


/* Occupied RAM at the point we are scannig it */
static int
is_precious_ram(unsigned long addr)
{
  extern int _start, _end;

  if ((unsigned long)&_start <= addr && addr <= (unsigned long)&_end)
    return 1;

#ifdef IMAGE_MODE
  unsigned i, c = _module_info_end - _module_info_start;
  if ((unsigned long)_module_info_start <= addr
       && addr <= (unsigned long)_module_info_end)
    return 1;

  if ((unsigned long)_modules_mbi_start <= addr
      && addr <= (unsigned long)_modules_mbi_end)
    return 1;

  for (i = 0; i < c; ++i)
    if (_module_info_start[i].start <= addr
        && addr < _module_info_start[i].start + _module_info_start[i].size_uncompressed)
      return 1;
#endif

  return 0;
}

unsigned long
scan_ram_size(unsigned long base_addr, unsigned long max_scan_size_mb)
{
  // scan the RAM to find out the RAM size, note that at this point we have
  // two regions in RAM that we cannot touch, &_start - &_end and the
  // modules

  unsigned long offset;
  const unsigned long increment = 1 << 20; // must be multiple of (1 << 20)

  printf("  Scanning up to %ld MB RAM\n", max_scan_size_mb);

  // initialize memory points
  for (offset = increment; offset < (max_scan_size_mb << 20); offset += offset)
    if (!is_precious_ram(base_addr + offset))
      *(unsigned long *)(base_addr + offset) = 0;

  // write something at offset 0, does it appear elsewhere?
  *(unsigned long *)base_addr = 0x12345678;
  asm volatile("" : : : "memory");
  for (offset = increment; offset < (max_scan_size_mb << 20); offset += offset)
    if (*(unsigned long *)(base_addr + offset) == 0x12345678)
      return offset >> 20;

  return max_scan_size_mb;
}

/**
 * \brief  Startup, started from crt0.S
 */
/* entry point */
extern "C" void
startup(l4util_mb_info_t *mbi, l4_umword_t flag,
	void *realmode_si, ptab64_mem_info_t *ptab64_info);
void
startup(l4util_mb_info_t *mbi, l4_umword_t flag,
	void *realmode_si, ptab64_mem_info_t *ptab64_info)
{
  void *l4i;
  boot_info_t boot_info;
  l4util_mb_mod_t *mb_mod;

  if (!Platform_base::platform)
    {
      // will we ever see this?
      printf("No platform found, hangup.");
      while (1)
        ;
    }

  puts("\nL4 Bootstrapper");
  puts("  Build: #" BUILD_NR " " BUILD_DATE
#ifdef ARCH_x86
      ", x86-32"
#endif
#ifdef ARCH_amd64
      ", x86-64"
#endif
#ifdef __VERSION__
       ", " __VERSION__
#endif
      );

  regions.init(__regs, sizeof(__regs) / sizeof(__regs[0]), "regions");
  ram.init(__ram, sizeof(__ram) / sizeof(__ram[0]), "RAM",
           get_memory_limit(mbi));

#ifdef ARCH_amd64
  // add the page-table on which we're running in 64bit mode
  regions.add(Region::n(ptab64_info->addr, ptab64_info->addr + ptab64_info->size,
              ".bootstrap-ptab64", Region::Boot));
#else
  (void)ptab64_info;
#endif

#if defined(ARCH_x86) || defined(ARCH_amd64)

#ifdef REALMODE_LOADING
  /* create synthetic multi boot info */
  mbi = init_loader_mbi(realmode_si);
  (void)flag;
#else
  (void)realmode_si;
  assert(flag == L4UTIL_MB_VALID); /* we need to be multiboot-booted */
#endif

#elif defined(ARCH_arm)
  l4util_mb_info_t my_mbi;
  memset(&my_mbi, 0, sizeof(my_mbi));
  mbi = &my_mbi;

  (void)realmode_si;
  (void)flag;

#elif defined(ARCH_ppc32)
  (void)realmode_si;
  (void)flag;

  l4util_mb_info_t my_mbi;
  L4_drivers::Of_if of_if;

  printf("  Detecting ram size ...\n");
  unsigned long ram_size = of_if.detect_ramsize();
  printf("    Total memory size is %luMB\n", ram_size / (1024 * 1024));

  /* setup mbi and detect OF devices */
  memset(&my_mbi, 0, sizeof(my_mbi));
  mbi = &my_mbi;
  unsigned long drives_addr, drives_length;

  if (of_if.detect_devices(&drives_addr, &drives_length))
    {
      mbi->flags |= L4UTIL_MB_DRIVE_INFO;
      mbi->drives_addr   = drives_addr;
      mbi->drives_length = drives_length;
    }
  ram.add(Region::n(0x0, ram_size, ".ram", Region::Ram));

#elif defined(ARCH_sparc)
  l4util_mb_info_t my_mbi;
  memset(&my_mbi, 0, sizeof(my_mbi));
  mbi = &my_mbi;

  (void)realmode_si;
  (void)flag;
#else
#error Unknown arch!
#endif

  setup_memory_map(mbi);

  /* basically add the bootstrap binary to the allocated regions */
  init_regions();

  /* check command line */
  if (check_arg(mbi, "-no-sigma0"))
    sigma0 = 0;

  if (check_arg(mbi, "-no-roottask"))
    roottask = 0;

  if (const char *s = check_arg(mbi, "-modaddr"))
    _mod_addr = strtoul(s + 9, 0, 0);

  _mod_addr = l4_round_page(_mod_addr);

#ifdef IMAGE_MODE
  construct_mbi(mbi);
#endif

  /* move vbe and ctrl structures to a known location, it might be in the
   * way when moving modules around */
#if defined(ARCH_x86) || defined(ARCH_amd64)
  if (mbi->flags & L4UTIL_MB_VIDEO_INFO)
    {
      if (mbi->vbe_mode_info)
	{
	  memcpy(&__mb_vbe, L4_VOID_PTR(mbi->vbe_mode_info),
		 sizeof(l4util_mb_vbe_mode_t));
	  mbi->vbe_mode_info = (l4_addr_t)&__mb_vbe;
	}

      if (mbi->vbe_ctrl_info)
	{
	  memcpy(&__mb_ctrl, L4_VOID_PTR(mbi->vbe_ctrl_info),
		 sizeof(l4util_mb_vbe_ctrl_t));
	  mbi->vbe_ctrl_info = (l4_addr_t)&__mb_ctrl;
	}
    }
#endif

  /* We need at least two boot modules */
  assert(mbi->flags & L4UTIL_MB_MODS);
  /* We have at least the L4 kernel and the first user task */
  assert(mbi->mods_count >= 2);
  assert(mbi->mods_count <= MODS_MAX);

  /* we're just a GRUB-booted kernel! */
  add_boot_modules_region(mbi);

  if (_mod_addr)
    move_modules(mbi, _mod_addr);

  if (const char *s = get_cmdline(mbi))
    {
      /* patch modules with content given at command line */
      while ((s = check_arg_str((char *)s, "-patch=")))
	patch_module(&s, mbi);
    }


  add_elf_regions(mbi, kernel_module, Region::Kernel);

  if (sigma0)
    add_elf_regions(mbi, sigma0_module, Region::Sigma0);

  if (roottask)
    add_elf_regions(mbi, roottask_module, Region::Root);


  /* copy Multiboot data structures, we still need to a safe place
   * before playing with memory we don't own and starting L4 */
  mb_info = relocate_mbi(mbi, &boot_info.mbi_low, &boot_info.mbi_high);
  if (!mb_info)
    panic("could not copy multiboot info to memory below 4MB");

  mb_mod = (l4util_mb_mod_t*)mb_info->mods_addr;

  /* --- Shouldn't touch original Multiboot parameters after here. -- */

  /* setup kernel PART ONE */
  printf("  Loading ");
  print_module_name(L4_CONST_CHAR_PTR(mb_mod[kernel_module].cmdline),
      "[KERNEL]");
  putchar('\n');

  boot_info.kernel_start = load_elf_module(mb_mod + kernel_module);

  /* setup sigma0 */
  if (sigma0)
    {
      printf("  Loading ");
      print_module_name(L4_CONST_CHAR_PTR(mb_mod[sigma0_module].cmdline),
			 "[SIGMA0]");
      putchar('\n');

      boot_info.sigma0_start = load_elf_module(mb_mod + sigma0_module);
      boot_info.sigma0_stack = (l4_addr_t)sigma0_init_stack
                               + sizeof(sigma0_init_stack);
    }

  /* setup roottask */
  if (roottask)
    {

      printf("  Loading ");
      print_module_name(L4_CONST_CHAR_PTR(mb_mod[roottask_module].cmdline),
			 "[ROOTTASK]");
      putchar('\n');

      boot_info.roottask_start = load_elf_module(mb_mod + roottask_module);
      boot_info.roottask_stack = (l4_addr_t)roottask_init_stack
	                         + sizeof(roottask_init_stack);
    }

  /* setup kernel PART TWO (special kernel initialization) */
  l4i = find_kip();

#if defined(ARCH_x86) || defined(ARCH_amd64) || defined(ARCH_ppc32)
  /* setup multi boot info structure for kernel */
  l4util_mb_info_t kernel_mbi;
  kernel_mbi = *mb_info;
  kernel_mbi.flags = L4UTIL_MB_MEMORY;
  if (mb_mod[kernel_module].cmdline)
    {
      kernel_mbi.cmdline = mb_mod[kernel_module].cmdline;
      kernel_mbi.flags  |= L4UTIL_MB_CMDLINE;
    }
#endif

  regions.optimize();
  regions.dump();

  if (char *c = check_arg(mbi, "-presetmem="))
    {
      unsigned fill_value = strtoul(c + 11, NULL, 0);
      fill_mem(fill_value);
    }

  L4_kernel_options::Options *lko = find_kopts(l4i);
  kcmdline_parse(L4_CONST_CHAR_PTR(mb_mod[kernel_module].cmdline), lko);
  lko->uart   = kuart;
  lko->flags |= kuart_flags;


  /* setup the L4 kernel info page before booting the L4 microkernel:
   * patch ourselves into the booter task addresses */
  unsigned long api_version = get_api_version(l4i);
  unsigned major = api_version >> 24;
  printf("  API Version: (%x) %s\n", major, (major & 0x80)?"experimental":"");
  switch (major)
    {
    case 0x02: // Version 2 API
    case 0x03: // Version X.0 and X.1
    case 0x87: // Fiasco
      init_kip_v2(l4i, &boot_info, mb_info, &ram, &regions);
      break;
    case 0x84:
    case 0x04:
      init_kip_v4(l4i, &boot_info, mb_info, &ram, &regions);
      break;
    default:
      panic("cannot boot a kernel with unknown api version %lx\n", api_version);
      break;
    }

  printf("  Starting kernel ");
  print_module_name(L4_CONST_CHAR_PTR(mb_mod[kernel_module].cmdline),
		    "[KERNEL]");
  printf(" at "l4_addr_fmt"\n", boot_info.kernel_start);

#if defined(ARCH_x86)
  asm volatile
    ("pushl $exit ; jmp *%3"
     :
     : "a" (L4UTIL_MB_VALID),
       "b" (&kernel_mbi),
       "S" (realmode_si),
       "r" (boot_info.kernel_start));

#elif defined(ARCH_amd64)

  asm volatile
    ("push $exit; jmp *%2"
     :
     : "S" (L4UTIL_MB_VALID), "D" (&kernel_mbi), "r" (boot_info.kernel_start));

#elif defined(ARCH_arm)
  typedef void (*startup_func)(void);
  startup_func f = (startup_func)boot_info.kernel_start;
  f();

#elif defined(ARCH_ppc32)

  init_kip_v2_arch((l4_kernel_info_t*)l4i);
  printf("CPU at %lu Khz/Bus at %lu Hz\n",
         ((l4_kernel_info_t*)l4i)->frequency_cpu,
         ((l4_kernel_info_t*)l4i)->frequency_bus);
  typedef void (*startup_func)(l4util_mb_info_t *, unsigned long);
  startup_func f = (startup_func)boot_info.kernel_start;
  of_if.boot_finish();
  f(&kernel_mbi, of_if.get_prom());

#elif defined(ARCH_sparc)

  printf("ENTER THE KERNEL!\n");
  asm volatile("or %%g0,%0,%%g2\n\t"
               "jmpl %%g2,%%g0\n\t"
               "nop\n\t" : : "r"(boot_info.kernel_start));

#else

#error "How to enter the kernel?"

#endif

  /*NORETURN*/
}

static int
l4_exec_read_exec(void * handle,
		  l4_addr_t file_ofs, l4_size_t file_size,
		  l4_addr_t mem_addr, l4_addr_t /*v_addr*/,
		  l4_size_t mem_size,
		  exec_sectype_t section_type)
{
  exec_task_t *exec_task = (exec_task_t*)handle;
  if (!mem_size)
    return 0;

  if (! (section_type & EXEC_SECTYPE_ALLOC))
    return 0;

  if (! (section_type & (EXEC_SECTYPE_ALLOC|EXEC_SECTYPE_LOAD)))
    return 0;

  if (mem_addr < exec_task->begin)
    exec_task->begin = mem_addr;
  if (mem_addr + mem_size > exec_task->end)
    exec_task->end = mem_addr + mem_size;

  if (Verbose_load)
    printf("    [%p-%p]\n", (void *) mem_addr, (void *) (mem_addr + mem_size));

  if (!ram.contains(Region::n(mem_addr, mem_addr + mem_size)))
    {
      printf("To be loaded binary region is out of memory region.\n");
      printf(" Binary region: %lx - %lx\n", mem_addr, mem_addr + mem_size);
      dump_ram_map();
      panic("Binary outside memory");
    }

  memcpy((void *) mem_addr,
         (char*)(exec_task->mod_start) + file_ofs, file_size);
  if (file_size < mem_size)
    memset((void *) (mem_addr + file_size), 0, mem_size - file_size);


  Region *f = regions.find(mem_addr);
  if (!f)
    {
      printf("could not find %lx\n", mem_addr);
      regions.dump();
      panic("Oops: region for module not found\n");
    }

  f->name(exec_task->mod->cmdline
	       ? L4_CONST_CHAR_PTR(exec_task->mod->cmdline)
	       :  ".[Unknown]");
  return 0;
}

static int
l4_exec_add_region(void * handle,
		  l4_addr_t /*file_ofs*/, l4_size_t /*file_size*/,
		  l4_addr_t mem_addr, l4_addr_t v_addr,
		  l4_size_t mem_size,
		  exec_sectype_t section_type)
{
  exec_task_t *exec_task = (exec_task_t*)handle;

  if (!mem_size)
    return 0;

  if (! (section_type & EXEC_SECTYPE_ALLOC))
    return 0;

  if (! (section_type & (EXEC_SECTYPE_ALLOC|EXEC_SECTYPE_LOAD)))
    return 0;

  regions.add(Region::n(mem_addr, mem_addr + mem_size,
             exec_task->mod->cmdline
	       ? L4_CONST_CHAR_PTR(exec_task->mod->cmdline)
	       :  ".[Unknown]", Region::Type(exec_task->type),
	       mem_addr == v_addr ? 1 : 0));
  return 0;
}
