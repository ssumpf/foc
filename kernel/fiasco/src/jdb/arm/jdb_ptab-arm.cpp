IMPLEMENTATION [arm]:

#include "paging.h"
#include "simpleio.h"

// -----------------------------------------------------------------------
IMPLEMENTATION [arm && armv5]:

PRIVATE inline
bool
Jdb_ptab::is_cached(Mword entry, unsigned level)
{
  if (level == 0 && (entry & 3) != 2)
    return true; /* No caching options on PDEs */
  return (entry & Page::Cache_mask) == Page::CACHEABLE;
}

PRIVATE inline
bool
Jdb_ptab::is_executable(Mword entry)
{
  (void)entry;
  return 1;
}

PRIVATE inline
char
Jdb_ptab::ap_char(unsigned ap)
{
  return ap & 0x2 ? (ap & 0x1) ? 'w' : 'r'
                  : (ap & 0x1) ? 'W' : 'R';
}


// -----------------------------------------------------------------------
IMPLEMENTATION [arm && (armv6 || armv7) && !arm_lpae]:

PRIVATE inline
bool
Jdb_ptab::is_cached(Mword entry, unsigned level)
{
  if (level == 0)
    {
      if ((entry & 3) == 2)
        return (entry & Page::Section_cache_mask) == Page::Section_cachable_bits;
      return true;
    }

  return (entry & Page::Cache_mask) == Page::CACHEABLE;
}

PRIVATE inline
bool
Jdb_ptab::is_executable(Mword entry)
{
  return (entry & 3) == 2 || (entry & 3) == 1;
}

PRIVATE inline
char
Jdb_ptab::ap_char(unsigned ap)
{
  switch (ap & 0x23)
  {
    case    0: return '-';
    case    1: return 'W';
    case    2: return 'r';
    case    3: return 'w';
    case 0x21: return 'R';
    case 0x22: case 0x23: return 'r';
    default: return '?';
  };
}

// -----------------------------------------------------------------------
IMPLEMENTATION [arm && !arm_lpae]:

IMPLEMENT
void
Jdb_ptab::print_entry(Pte_ptr const &entry)
{
  if (dump_raw)
    printf("%08lx", *entry.pte);
  else
    {
      if (!entry.is_valid())
	{
	  putstr("    -   ");
	  return;
	}
      Address phys = entry_phys(entry);

      unsigned t = *entry.pte & 0x03;
      unsigned ap = *entry.pte >> 4;
      char ps;
      if (entry.level == 0)
	switch (t)
	  {
	  case 1: ps = 'C'; break;
	  case 2: ps = 'S'; ap = *entry.pte >> 10; break;
	  case 3: ps = 'F'; break;
	  default: ps = 'U'; break;
	  }
      else
	switch (t)
	  {
	  case 1: ps = 'l'; break;
	  case 2: ps = 's'; break;
	  case 3: ps = 't'; break;
	  default: ps = 'u'; break;
	  }

      printf("%05lx%s%c", phys >> Config::PAGE_SHIFT,
                          is_cached(*entry.pte, entry.level)
                           ? "-" : JDB_ANSI_COLOR(lightblue) "n" JDB_ANSI_END,
                          ps);
      if (entry.level == 0 && t != 2)
        putchar('-');
      else
	printf("%s%c" JDB_ANSI_END,
               is_executable(*entry.pte) ? "" : JDB_ANSI_COLOR(red),
	       ap_char(ap));
    }
}

// -----------------------------------------------------------------------
IMPLEMENTATION [arm && arm_lpae]:

PRIVATE inline
bool
Jdb_ptab::is_cached(Pte_ptr const &entry)
{
  if (!entry.is_leaf())
    return true;
  return (*entry.pte & Page::Cache_mask) == Page::CACHEABLE;
}

PRIVATE inline
bool
Jdb_ptab::is_executable(Mword entry)
{
  (void)entry;
  return 1;
}

PRIVATE inline
char
Jdb_ptab::ap_char(Mword entry)
{
  switch ((entry >> 6) & 3)
  {
    case 0: return 'W';
    case 1: return 'w';
    case 2: return 'R';
    case 3: default: return 'r';
  };
}

IMPLEMENT
void
Jdb_ptab::print_entry(Pte_ptr const &entry)
{
  if (dump_raw)
    printf("%08lx", (unsigned long)*entry.pte);
  else
    {
      if (!entry.is_valid())
	{
	  putstr("    -   ");
	  return;
	}
      Address phys = entry_phys(entry);

      unsigned t = (*entry.pte & 2) >> 1;

      char ps;
      if (entry.level == 0)
        switch (t)
          {
          case 0: ps = 'G'; break;
          case 1: ps = 'P'; break;
          }
      else if (entry.level == 1)
        switch (t)
          {
          case 0: ps = 'M'; break;
          case 1: ps = 'P'; break;
          }
      else
        switch (t)
          {
          case 0: ps = '?'; break;
          case 1: ps = 'p'; break;
          }

      printf("%05lx%s%c", phys >> Config::PAGE_SHIFT,
                          is_cached(entry)
                           ? "-" : JDB_ANSI_COLOR(lightblue) "n" JDB_ANSI_END,
                          ps);
      if (!entry.is_leaf())
        putchar('-');
      else
        printf("%s%c" JDB_ANSI_END,
               is_executable(*entry.pte) ? "" : JDB_ANSI_COLOR(red),
               ap_char(*entry.pte));
    }
}
