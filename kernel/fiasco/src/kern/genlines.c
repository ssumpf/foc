
/* Extract debug lines info from an ELF binary stab section */

/*
 * Adam: Note, this file works on 32-bit files only.
 */

#include <stdio.h>
#include <elf.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PAGESIZE		4096
#define MAX_PAGES		512
#define LIN_PER_PAGE		(PAGESIZE/sizeof(Stab_entry))

typedef struct 
{
  unsigned int  n_strx;         /* index into string table of name */
  unsigned char n_type;         /* type of symbol */
  unsigned char n_other;        /* misc info (usually empty) */
  unsigned short n_desc;        /* description field */
  unsigned int   n_value;        /* value of symbol */
} __attribute__ ((packed)) Stab_entry;

typedef struct
{
  unsigned int  addr;
  unsigned short line;
} __attribute__ ((packed)) Stab_line;

const unsigned str_max = MAX_PAGES*PAGESIZE;
const unsigned lin_max = MAX_PAGES*PAGESIZE/sizeof(Stab_line);

static char *str_field;
static Stab_line *lin_field;
static unsigned str_end;
static unsigned lin_end;
static unsigned func_offset;
static int have_func;

static Elf32_Shdr*
elf_sh_lookup(unsigned char *elf_image, int n)
{
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)elf_image;

  return (n < ehdr->e_shnum)
    ? (Elf32_Shdr*)(elf_image + ehdr->e_shoff + n*ehdr->e_shentsize)
    : 0;
}

static int
search_str(const char *str, unsigned len, unsigned *idx)
{
  const char *c_next, *c, *d;

  if (!str_field || !*str_field)
    return 0;

  for (c_next=str_field; ; )
    {
      // goto end of string
      for (; *c_next; c_next++)
	;

      // compare strings
      for (c=c_next, d=str+len; d>=str && (*c == *d); c--, d--)
	;
      
      if (d<str)
	{
	  *idx = c+1-str_field;
	  // found (could be a substring)
	  return 1;
	}

      // test for end-of-field
      if (!*++c_next)
	// end of field -- not found
	return 0;
    }
}

static int
add_str(const char *str, unsigned len, unsigned *idx)
{
  // if still included, don't include anymore
  if (search_str(str, len, idx))
    return 1;

  // check if str_field is big enough
  if (str_end+len+1 >= str_max)
    {
      fprintf(stderr, "Not enough memory for debug lines (str space)\n");
      return 0;
    }

  *idx = str_end;
  
  // add string to end of str_field
  strcpy(str_field+str_end, str);
  str_end += len+1;

  // terminate string field
  str_field[str_end] = '\0';

  return 1;
}

// see documentation in "info stabs"
static int
add_entry(const Stab_entry *se, const char *se_str)
{
  const char *se_name = se_str + se->n_strx;
  int se_name_len = strlen(se_name);
  unsigned se_name_idx = 0;

  if (se_name_len
      && ((se->n_type == 0x64) || (se->n_type == 0x84)))
    {
      if (!add_str(se_name, se_name_len, &se_name_idx))
	return 0;
    }

  if (lin_end >= lin_max)
    {
      fprintf(stderr, "Not enough temporary memory for lines (lin space)\n");
      return 0;
    }
 
  switch (se->n_type)
    {
    case 0x24: // N_FUN: function name
      func_offset = se->n_value;		// start address of function
      have_func   = 1;
      break;
    case 0x44: // N_SLINE: line number in text segment
      if (have_func && (se->n_desc < 0xfffd)) // sanity check
	{
	  // Search last SLINE entry and compare addresses. If they are
	  // same, overwrite the last entry because we display only one
	  // line number per address
	  unsigned addr = se->n_value + func_offset;

	  unsigned l = lin_end;
	  while ((l > 0) && (lin_field[l-1].line>=0xfffd))
	    l--;
	  
	  if ((l > 0) && (lin_field[l-1].addr == addr))
	    {
	      // found, delete array entry
	      while (l < lin_end)
		{
		  lin_field[l-1] = lin_field[l];
		  l++;
		}
	      lin_end--;
	    }
	  // append
	  lin_field[lin_end].addr = addr;
     	  lin_field[lin_end].line = se->n_desc;
	  lin_end++;
	}
      break;
    case 0x64: // N_SO: main file name / directory
      if (se_name_len)
	{
	  if (lin_end>0 && lin_field[lin_end-1].line == 0xffff)
	    {
	      // mark last entry as directory, this is a file name
	      lin_field[lin_end-1].line = 0xfffe;
	    }
	}
      else
	have_func = 0;
      // fall through
    case 0x84: // N_SOL: name of include file
      if (se_name_len)
	{
	  int type = 0xffff;
	  char c = se_name[se_name_len-1];
	  
	  if (c=='h' || c=='H')
	    type = 0xfffd; // mark as header file
	  
	  lin_field[lin_end].addr = se_name_idx;
	  lin_field[lin_end].line = type;
	  lin_end++;
	}
      break;
    }

  return 1;
}

static int
add_section(const Stab_entry *se, const char *str, unsigned n)
{
  unsigned int i;

  func_offset = 0;
  have_func   = 0;

  for (i=0; i<n; i++)
    if (!add_entry(se++, str))
      return 0;

  return 1;
}

int
extract_lines(unsigned char *elf_image) 
{
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)elf_image;
  Elf32_Shdr *sh_sym, *sh_str;
  const char *strtab;
  unsigned int i;

  // search contiguous free pages for temporary memory
  str_end   = lin_end = 0;
  lin_field = (Stab_line*)malloc(lin_max);
  str_field = (char*)malloc(str_max);

  str_field[0] = '\0';

  if (ehdr->e_shstrndx == SHN_UNDEF)
    {
      fprintf(stderr, "Error in ELF header\n");
      return -3;
    }

  if (0 == (sh_str = elf_sh_lookup(elf_image, ehdr->e_shstrndx)))
    {
      fprintf(stderr, "Error in ELF stab section\n");
      return -3;
    }

  strtab = (const char*)(elf_image + sh_str->sh_offset);

  for (i=0; i<ehdr->e_shnum; i++)
    {
      sh_sym = elf_sh_lookup(elf_image, i);
      if (   (   sh_sym->sh_type == SHT_PROGBITS
	      || sh_sym->sh_type == SHT_STRTAB)
	  && (!strcmp(sh_sym->sh_name + strtab, ".stab")))
	{
	  if (0 == (sh_str = elf_sh_lookup(elf_image, sh_sym->sh_link)))
	    {
	      fprintf(stderr, "Error in ELF stab section\n");
	      return -3;
	    }

	  if (!add_section((const Stab_entry*)(elf_image+sh_sym->sh_offset),
			   (const char*)      (elf_image+sh_str->sh_offset),
			   sh_sym->sh_size/sh_sym->sh_entsize))
      	    return -3;
	}
    }

  if (lin_end > 0)
    {
      // add terminating entry
      if (lin_end >= lin_max)
	{
	  fprintf(stderr, "Not enough temporary memory for lines "
			  "(terminating line entry)\n");
      	  return -3;
	}
      lin_field[lin_end].addr = 0;
      lin_field[lin_end].line = 0;
      lin_end++;

      // string offset relativ to begin of lines
      for (i=0; i<lin_end; i++)
	if (lin_field[i].line >= 0xfffd)
	  lin_field[i].addr += lin_end*sizeof(Stab_line);

      fwrite(lin_field, lin_end*sizeof(Stab_line), 1, stdout);
      fwrite(str_field, str_end, 1, stdout);

      return 0;
    }

  fprintf(stderr, "No lines stored\n");
  return -3;
}

int
main(int argc, char **argv)
{
  int fd;
  unsigned size;
  unsigned char *image;

  if (argc != 2)
    {
      fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
      return -1;
    }

  if ((fd = open(argv[1], O_RDONLY)) < 0)
    {
      fprintf(stderr, "Cannot open %s\n", argv[1]);
      return -2;
    }

  size = lseek(fd, 0, SEEK_END);

  if ((image = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0)) 
      == (unsigned char*)-1)
    {
      fprintf(stderr, "Cannot map ELF file\n");
      return -3;
    }

  return extract_lines(image);
}

