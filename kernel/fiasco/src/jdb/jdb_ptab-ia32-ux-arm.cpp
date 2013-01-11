IMPLEMENTATION [ia32,ux]:

#include "paging.h"

unsigned Jdb_ptab::max_pt_level = Pdir::Depth;

static unsigned long _leaf_check[]
= { Pt_entry::Pse_bit | Pt_entry::Valid, 0 };
static unsigned long next_level_mask[]
= { 0xfffff000, 0 };


static inline bool leaf_check(unsigned long entry, unsigned level)
{ return (_leaf_check[level] & entry) == _leaf_check[level]; }

IMPLEMENT
unsigned
Jdb_ptab::entry_is_pt_ptr(Mword entry, unsigned level,
                          unsigned *entries, unsigned *next_level)
{
  if (leaf_check(entry, level))
    return 0;
  *entries = Ptab::Level<Pdir::Traits, Pdir::Depth>::length(level);
  *next_level = level+1;
  return 1;
}

IMPLEMENT
Address
Jdb_ptab::entry_phys(Mword entry, unsigned level)
{
  if (!leaf_check(entry, level))
    return entry & next_level_mask[level];

  return Ptab::Level<Pdir::Traits, Pdir::Depth>::addr(level, entry);
}

IMPLEMENT inline NEEDS ["paging.h"]
unsigned
Jdb_ptab::entry_valid(Mword entry, unsigned)
{ return entry & Pt_entry::Valid; }

IMPLEMENT
void
Jdb_ptab::print_entry(Mword entry, unsigned level)
{
  if (dump_raw)
    {
      printf(L4_PTR_FMT, entry);
      return;
    }

  if (!entry_valid(entry,level))
    {
      putstr("    -   ");
      return;
    }

  Address phys = entry_phys(entry, level);

  if (level == Pdir::Super_level && entry & Pt_entry::Pse_bit)
    printf((phys >> 20) > 0xFF
	    ? "%03lX/4" : " %02lX/4", phys >> 20);
  else
    printf((phys >> Config::PAGE_SHIFT) > 0xFFFF
	    ? "%05lx" : " %04lx", phys >> Config::PAGE_SHIFT);

  putchar(((cur_pt_level>=max_pt_level || (entry & Pt_entry::Pse_bit)) &&
	 (entry & Pt_entry::Cpu_global)) ? '+' : '-');
  printf("%s%c%s", entry & Pt_entry::Noncacheable ? JDB_ANSI_COLOR(lightblue) : "",
                   entry & Pt_entry::Noncacheable
	            ? 'n' : (entry & Pt_entry::Write_through) ? 't' : '-',
                   entry & Pt_entry::Noncacheable ? JDB_ANSI_END : "");
  putchar(entry & Pt_entry::User
	    ? (entry & Pt_entry::Writable) ? 'w' : 'r'
	    : (entry & Pt_entry::Writable) ? 'W' : 'R');
}

PUBLIC
unsigned long
Jdb_ptab::rows() const
{ return entries/8; }

IMPLEMENTATION [ia32,ux,arm]:

#if 0
// calculate row from virtual address
PRIVATE
unsigned
Jdb_ptab::disp_virt_to_r(Address v)
{
  v = Ptab::Level<Pdir::Traits, Pdir::Depth>::index(cur_pt_level, v >> Pdir::Va::Shift);
  return v / (cols()-1);
}

// calculate column from virtual address
PRIVATE
unsigned
Jdb_ptab::disp_virt_to_c(Address v)
{
  v = Ptab::Level<Pdir::Traits, Pdir::Depth>::index(cur_pt_level, v >> Pdir::Va::Shift);
  return (v % (cols()-1)) + 1;
}
#endif

PRIVATE
Address
Jdb_ptab::disp_virt(unsigned row, unsigned col)
{
  Pdir::Va e((col-1) + (row * (cols()-1)));
  e <<= Ptab::Level<Pdir::Traits, Pdir::Depth>::shift(cur_pt_level);
  return Virt_addr(e).value() + virt_base;
}

PUBLIC
void
Jdb_ptab::print_statline(unsigned long row, unsigned long col)
{
  unsigned long sid = Kobject_dbg::pointer_to_id(_task);

  if (cur_pt_level<max_pt_level)
    Jdb::printf_statline("pdir", "<Space>=mode <CR>=goto ptab/superpage",
                         "<" L4_PTR_FMT "> task D:%lx", disp_virt(row,col), sid);
  else // PT_MODE
    Jdb::printf_statline("ptab", "<Space>=mode <CR>=goto page",
                         "<" L4_PTR_FMT "> task D:%lx", disp_virt(row,col), sid);
}
