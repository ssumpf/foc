#ifndef __PAGETABLE_ARCH_DEFS_H__
#define __PAGETABLE_ARCH_DEFS_H__

struct Fld_coarse
{
  mword_t type        : 2; // must be 0x01
  mword_t imp         : 3; // Implementation defined
  mword_t domain      : 4;
  mword_t sbz         : 1; // should be zero
  mword_t ptb         : MWORD_BITS - 10; // ptb >> 10
};

struct Fld_fine
{
  mword_t type        : 2; // must be 0x03
  mword_t imp         : 3; // Implementation defined
  mword_t domain      : 4; 
  mword_t sbz         : 3; // should be zero
  mword_t ptb         : MWORD_BITS - 12;// ptb >> 12
};

struct Fld_section
{
  mword_t type        : 2; // must be 0x02
  mword_t cb          : 2; // cachable and bufferable
  mword_t imp         : 1; // implementation defined
  mword_t domain      : 4;
  mword_t sbz1        : 1; // should be zero
  mword_t ap          : 2; // protection
  mword_t sbz2        : 8; // should be zero 2
  mword_t sb          : MWORD_BITS - 20; // sb >> 20
};

struct Fld_empty
{
  mword_t type        : 2; // must bew 0x00
  mword_t udef        : MWORD_BITS - 2;
};

union Fld 
{
  Fld_empty   e;
  Fld_section s;
  Fld_coarse  c;
  Fld_fine    f;
  mword_t     raw;
};


struct Sld_large
{
  mword_t type        : 2; // must be 0x01
  mword_t cb          : 2; // cache & buffer
  mword_t ap0         : 2;
  mword_t ap1         : 2;
  mword_t ap2         : 2;
  mword_t ap3         : 2;
  mword_t sbz         : 4; // should be zwero
  mword_t pb          : MWORD_BITS - 16;
};

struct Sld_small
{
  mword_t type        : 2; // must be 0x02
  mword_t cb          : 2; // cache & buffer
  mword_t ap0         : 2;
  mword_t ap1         : 2;
  mword_t ap2         : 2;
  mword_t ap3         : 2;
  mword_t pb          : MWORD_BITS - 12;
};

struct Sld_tiny
{
  mword_t type        : 2; // must be 0x03
  mword_t cb          : 2; // cache & buffer
  mword_t ap          : 2; 
  mword_t sbz         : 4; 
  mword_t pb          : MWORD_BITS - 10;
};

struct Sld_empty
{
  mword_t type        : 2; // must be 0x00
  mword_t udef        : MWORD_BITS - 2;
};

union Sld
{
  Sld_empty e;
  Sld_tiny  t;
  Sld_small s;
  Sld_large l;
  mword_t   raw;
};

#endif // __PAGETABLE_ARCH_DEFS_H__
