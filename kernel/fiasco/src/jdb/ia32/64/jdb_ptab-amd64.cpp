IMPLEMENTATION [amd64]:

#include "paging.h"

unsigned Jdb_ptab::max_pt_level = Pdir::Depth;

IMPLEMENT inline NEEDS ["paging.h"]
unsigned
Jdb_ptab::entry_is_pt_ptr(Mword entry, unsigned level,
                          unsigned *entries, unsigned *next_level)
{
  if (level > Pdir::Super_level || entry & Pt_entry::Pse_bit)
    return 0;
  *entries = Ptab::Level<Pdir::Traits, Pdir::Depth>::length(level);
  *next_level = level+1;
  return 1;
}

IMPLEMENT inline NEEDS ["paging.h"]
Address
Jdb_ptab::entry_phys(Mword entry, unsigned level)
{
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
      putstr("        -       ");
      return;
    }

  Address phys = entry_phys(entry, level);

  if (level == Pdir::Super_level && entry & Pt_entry::Pse_bit)
    printf((phys >> 20) > 0xFF
	   ? "       %03lx/2" : "        %02lx/2", phys >> 20);
  else
    printf((phys >> Config::PAGE_SHIFT) > 0xFFFF
           ? "%13lx" : "         %04lx", phys >> Config::PAGE_SHIFT);

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
{  return entries/4; }

PRIVATE
Address
Jdb_ptab::disp_virt(unsigned long row, unsigned col)
{
  Pdir::Va e(Mword(col-1) + (Mword(row) * Mword(cols()-1)));
  e <<= Ptab::Level<Pdir::Traits, Pdir::Depth>::shift(cur_pt_level);
  return Virt_addr(e).value() + virt_base;
}

PUBLIC
void
Jdb_ptab::print_statline(unsigned long row, unsigned long col)
{
  if (cur_pt_level == 0)
    {
      Jdb::printf_statline("pml4", "<Space>=mode <CR>=goto pdp",
	  "<" L4_PTR_FMT "> task %p", disp_virt(row,col), _task);
    }
  else if (cur_pt_level == 1)
    {
      Jdb::printf_statline("pdp", "<Space>=mode <CR>=goto pdir",
	  "<" L4_PTR_FMT "> task %p", disp_virt(row,col), _task);
    }
  else if (cur_pt_level == 2)
    {
      Jdb::printf_statline("pdir", "<Space>=mode <CR>=goto ptab/superpage",
	  "<" L4_PTR_FMT "> task %p", disp_virt(row,col), _task);
    }
  else // PT_MODE
    {
      Jdb::printf_statline("ptab", "<Space>=mode <CR>=goto page",
	  "<" L4_PTR_FMT "> task %p", disp_virt(row,col), _task);
    }
}
