/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "panic.h"
#include <l4/util/l4_macros.h>

#include "region.h"
#include "module.h"

bool
Region_list::test_fit(unsigned long long start, unsigned long long _size)
{
  Region r(start, start + _size);
  for (Region const *c = _reg; c < _end; ++c)
    {
      if (0)
        printf("test [%p-%p] [%llx-%llx]\n",
               (char *)start, (char *)start + _size, c->begin(), c->end());

      if (c->overlaps(r))
	return false;
    }
  return true;
}

unsigned long long
Region_list::next_free(unsigned long long start)
{
  unsigned long long s = ~0ULL, e = ~0ULL;
  for (Region const *c = _reg; c < _end; ++c)
    {
      e = c->end();
      if (e > start && e < s)
        s = e;
    }

  if (s == ~0ULL)
    return e;

  return s;
}

unsigned long long
Region_list::find_free(Region const &search, unsigned long long _size,
                       unsigned align)
{
  unsigned long long start = search.begin();
  unsigned long long end   = search.end();
  while (1)
    {
      start = (start + (1ULL << align) -1) & ~((1ULL << align)-1);

      if (start + _size - 1 > end)
	return 0;

      if (0)
        printf("try start %p\n", (void *)start);

      if (test_fit(start, _size))
	return start;

      start = next_free(start);
    }
}

void
Region_list::add_nolimitcheck(Region const &region, bool may_overlap)
{
  Region const *r;

  /* Do not add empty regions */
  if (region.begin() == region.end())
    return;

  if (_end >= _max)
    panic("Bootstrap: %s: Region overflow\n", __func__);

  if (!may_overlap && (r = find(region))
      // sometimes there are smaller regions in regions of the same type
      && !(   r->contains(region)
           && region.type() == r->type()))
    {
      printf("  New region for list %s:\t", _name);
      region.vprint();
      printf("  overlaps with:         \t");
      r->vprint();

      dump();
      panic("region overlap");
    }

  *_end = region;
  ++_end;
}

void
Region_list::add(Region const &region, bool may_overlap)
{
  Region mem = region;

  if (mem.invalid())
    {
      printf("  WARNING: trying to add invalid region to %s list.\n", _name);
      return;
    }

  if (mem.begin() >= _upper_limit)
    {
      printf("  Dropping %s region ", _name);
      mem.print();
      printf(" due to %lld MB limit\n", _upper_limit >> 20);
      return;
    }

  if (mem.end() >= _upper_limit - 1)
    {
      printf("  Limiting %s region ", _name);
      mem.print();
      mem.end(_upper_limit - 1);
      printf(" to ");
      mem.print();
      printf(" due to %lld MB limit\n", _upper_limit >> 20);
    }

  add_nolimitcheck(mem, may_overlap);
}

Region *
Region_list::find(Region const &o)
{
  for (Region *c = _reg; c < _end; ++c)
    if (c->overlaps(o))
      return c;

  return 0;
}

Region *
Region_list::contains(Region const &o)
{
  for (Region *c = _reg; c < _end; ++c)
    if (c->contains(o))
      return c;

  return 0;
}

void
Region::print() const
{
  printf("  [%9llx, %9llx] {%9llx}", begin(), end(), end() - begin() + 1);
}

void
Region::vprint() const
{
  static char const *types[] = {"Gap   ", "Kern  ", "Sigma0",
                                "Boot  ", "Root  ", "Arch  ", "Ram   " };
  printf("  ");
  print();
  printf(" %s ", types[type()]);
  if (name())
    {
      if (*name() == '.')
	printf("%s", name() + 1);
      else
	print_module_name(name(), "");
    }
  putchar('\n');
}

void
Region_list::dump()
{
  Region const *i;
  Region const *j;
  Region const *min_idx;
  unsigned long long min, mark = 0;

  printf("Regions of list '%s'\n", _name);
  for (i = _reg; i < _end; ++i)
    {
      min = ~0;
      min_idx = 0;
      for (j = _reg; j < _end; ++j)
	if (j->begin() < min && j->begin() >= mark)
	  {
	    min     = j->begin();
	    min_idx = j;
	  }
      if (!min_idx)
	printf("Check region dump\n");
      min_idx->vprint();
      mark = min_idx->begin() + 1;
    }
}

void
Region_list::swap(Region *a, Region *b)
{
  Region t = *a; *a = *b; *b = t;
}

void
Region_list::sort()
{
  if (end() - begin() < 2)
    return;
  bool swapped;

  Region *e = end() - 1;

  do
    {
      swapped = false;
      for (Region *c = begin(); c < e; ++c)
	{
	  Region *n = c; ++n;
	  if (*n < *c)
	    {
	      swap(c,n);
	      swapped = true;
	    }
	}
    }
  while (swapped);
}

void
Region_list::remove(Region *r)
{
  memmove(r, r+1, (end() - r - 1)*sizeof(Region));
  --_end;
}

void
Region_list::optimize()
{
  sort();
  Region *c = begin();
  while (c < end())
    {
      Region *n = c; ++n;
      if (n == end())
	return;

      if (n->type() == c->type() && n->sub_type() == c->sub_type()
	  && n->name() == c->name() &&
	  l4_round_page(c->end()) >= l4_trunc_page(n->begin()))
	{
	  c->end(n->end());
	  remove(n);
	}
      else
	++c;
    }
}

