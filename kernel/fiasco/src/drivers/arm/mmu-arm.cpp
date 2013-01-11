INTERFACE:

#include "mem.h"
#include "std_macros.h"

EXTENSION class Mmu
{
public:
  static void btc_flush();
  static void btc_inv();

  enum
  {
    Cache_line_size = 32,
    Cache_line_mask = Cache_line_size - 1,
    Icache_line_size = 32,
    Icache_line_mask = Icache_line_size - 1,
  };
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv5]:

IMPLEMENT static inline
template < unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::btc_flush()
{}

IMPLEMENT static inline
template < unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::btc_inv()
{}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && (armv6 || armv7)]:

IMPLEMENT static inline
template < unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::btc_flush()
{ asm volatile ("mcr p15, 0, %0, c7, c5, 6" : : "r" (0) : "memory"); }

IMPLEMENT static inline
template < unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::btc_inv()
{ asm volatile ("mcr p15, 0, %0, c7, c5, 0" : : "r" (0) : "memory"); }

// ---------------------------------------------------------------------------
IMPLEMENTATION [arm && arm920t]:

IMPLEMENT inline
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_cache(void const * /*start*/,
				       void const * /*end*/)
{
  flush_cache();
}

IMPLEMENT
template< unsigned long Flush_area , bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache(void const *start, void const *end)
{
  (void)start; (void)end;
  clean_dcache();
}

IMPLEMENT
template< unsigned long Flush_area , bool Ram >
void Mmu<Flush_area, Ram>::clean_dcache(void const *va)
{
  (void)va;
  clean_dcache();
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_dcache(void const *start, void const *end)
{
  (void)start; (void)end;
  flush_dcache();
}


IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::inv_dcache(void const *start, void const *end)
{
  // clean && invalidate dcache  ||| XXX: all
#if 1
  for (unsigned long index = 0; index < (1 << (32 - 26)); ++index)
    for (unsigned long seg = 0; seg < 256; seg += 32)
      asm volatile("mcr p15,0,%0,c7,c14,2" : : "r" ((index << 26) | seg));
#else
  // invalidate dcache --- all
  asm volatile("mcr p15,0,%0,c7,c6,0" : : "r" (0) : "memory");
#endif
}


// ---------------------------------------------------------------------------
IMPLEMENTATION [arm && (pxa || sa1100 || 926)]:

IMPLEMENT inline
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_cache(void const * /*start*/,
				       void const * /*end*/)
{
  flush_cache();
}

IMPLEMENT
template< unsigned long Flush_area , bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache(void const *start, void const *end)
{
  if (((Address)end) - ((Address)start) >= 8192)
    clean_dcache();
  else
    {
      asm volatile (
	  "    bic  %0, %0, %2 - 1         \n"
	  "1:  mcr  p15, 0, %0, c7, c10, 1 \n"
	  "    add  %0, %0, %2             \n"
	  "    cmp  %0, %1                 \n"
	  "    blo  1b                     \n"
	  "    mcr  p15, 0, %0, c7, c10, 4 \n" // drain WB
	  : : "r" (start), "r" (end), "i" (Cache_line_size)
	  );
    }
}

IMPLEMENT
template< unsigned long Flush_area , bool Ram >
void Mmu<Flush_area, Ram>::clean_dcache(void const *va)
{
  __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 1       \n"
                        : : "r"(va) : "memory");
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_dcache(void const *start, void const *end)
{
  if (((Address)end) - ((Address)start) >= 8192)
    flush_dcache();
  else
    {
      asm volatile (
	  "    bic  %0, %0, %2 - 1         \n"
	  "1:  mcr  p15, 0, %0, c7, c14, 1 \n"
	  "    add  %0, %0, %2             \n"
	  "    cmp  %0, %1                 \n"
	  "    blo  1b                     \n"
	  "    mcr  p15, 0, %0, c7, c10, 4 \n" // drain WB
	  : : "r" (start), "r" (end), "i" (Cache_line_size)
	  );
    }
}


IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::inv_dcache(void const *start, void const *end)
{
  asm volatile (
	  "    bic  %0, %0, %2 - 1         \n"
	  "1:  mcr  p15, 0, %0, c7, c6, 1  \n"
	  "    add  %0, %0, %2             \n"
	  "    cmp  %0, %1                 \n"
	  "    blo  1b                     \n"
	  : : "r" (start), "r" (end), "i" (Cache_line_size)
	  );
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && (mpcore || arm1136 || arm1176 || armca8 || armca9)]:

IMPLEMENT inline
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_cache(void const *start,
				       void const *end)
{
  __asm__ __volatile__ (
      "1:  mcr p15, 0, %[i], c7, c14, 1  \n"
      "    mcr p15, 0, %[i], c7, c5, 1   \n"
      "    add %[i], %[i], %[clsz]       \n"
      "    cmp %[i], %[end]              \n"
      "    blo 1b                        \n"
      : [i]     "=&r" (start)
      :         "0"   ((unsigned long)start & ~(Cache_line_size - 1)),
        [end]   "r"   (end),
	[clsz]  "i"   (Cache_line_size)
      : "r0", "memory");
  btc_inv();
  Mem::dsb();
}

IMPLEMENT inline
template< unsigned long Flush_area , bool Ram >
void Mmu<Flush_area, Ram>::clean_dcache(void const *va)
{
  Mem::dsb();
  __asm__ __volatile__ (
      "mcr p15, 0, %1, c7, c10, 1       \n" // Clean Data Cache Line (using MVA) Register
      :
      : "r" (0),
        "r"((unsigned long)va & ~(Cache_line_size - 1))
      : "memory");
}

IMPLEMENT inline
template< unsigned long Flush_area , bool Ram >
void Mmu<Flush_area, Ram>::clean_dcache(void const *start, void const *end)
{
  Mem::dsb();
  __asm__ __volatile__ (
      // arm1176 only: "    mcrr p15, 0, %2, %1, c12         \n"
      "1:  mcr p15, 0, %[i], c7, c10, 1   \n" // Clean Data Cache Line (using MVA) Register
      "    add %[i], %[i], %[clsz]        \n"
      "    cmp %[i], %[end]               \n"
      "    blo 1b                         \n"
      : [i]     "=&r" (start)
      :         "0"   ((unsigned long)start & ~(Cache_line_size - 1)),
        [end]   "r"   (end),
	[clsz]  "i"   (Cache_line_size)
      : "memory");
  btc_inv();
  Mem::dsb();
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_dcache(void const *start, void const *end)
{
  Mem::dsb();
  __asm__ __volatile__ (
      "1:  mcr p15, 0, %[i], c7, c14, 1 \n" // Clean and Invalidate Data Cache Line (using MVA) Register
      "    add %[i], %[i], %[clsz]      \n"
      "    cmp %[i], %[end]             \n"
      "    blo 1b                       \n"
      : [i]    "=&r" (start)
      :        "0"   ((unsigned long)start & ~(Cache_line_size - 1)),
        [end]  "r"   (end),
	[clsz] "i"   (Cache_line_size)
      : "memory");
  btc_inv();
  Mem::dsb();
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::inv_dcache(void const *start, void const *end)
{
  Mem::dsb();
  __asm__ __volatile__ (
      "1:  mcr p15, 0, %[i], c7, c6, 1  \n" // Invalidate Data Cache Line (using MVA) Register
      "    add %[i], %[i], %[clsz]      \n"
      "    cmp %[i], %[end]             \n"
      "    blo 1b                       \n"
      : [i]    "=&r" (start)
      :        "0"   ((unsigned long)start & ~(Cache_line_size - 1)),
        [end]  "r"   (end),
	[clsz] "i"   (Cache_line_size)
      : "memory");
  btc_inv();
  Mem::dsb();
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && (mpcore || arm1136 || arm1176)]:

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_cache()
{
  Mem::dsb();
  __asm__ __volatile__ (
      "    mcr p15, 0, r0, c7, c14, 0       \n" // Clean and Invalidate Entire Data Cache Register
      "    mcr p15, 0, r0, c7, c5, 0        \n" // Invalidate Entire Instruction Cache Register
      : : : "memory");
  btc_inv();
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::clean_dcache()
{
  Mem::dsb();
  __asm__ __volatile__ (
      "    mcr p15, 0, r0, c7, c10, 0       \n" // Clean Entire Data Cache Register
      : : : "memory");
  btc_inv();
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_dcache()
{
  Mem::dsb();
  __asm__ __volatile__ (
      "    mcr p15, 0, r0, c7, c14, 0       \n" // Clean and Invalidate Entire Data Cache Register
      : : : "memory");
  btc_inv();
}


//-----------------------------------------------------------------------------
INTERFACE [arm && (armca8 || armca9)]:

EXTENSION class Mmu
{
private:
  struct set_way_dcache_flush_op
  {
    void operator()(Mword v) const
    { // clean+inv data cache by set/way
      __asm__ __volatile__
        ( "mcr p15, 0, %0, c7, c14, 2" : : "r"(v) : "memory");
    }
  };

  struct set_way_dcache_clean_op
  {
    void operator()(Mword v) const
    { // clean data cache by set/way
      __asm__ __volatile__
        ( "mcr p15, 0, %0, c7, c10, 2" : : "r"(v) : "memory");
    }
  };
};

//-----------------------------------------------------------------------------
INTERFACE [arm && armca8]:

EXTENSION class Mmu
{
  enum
  {
    SET_SIZE_16KB = 1 << 12,
    SET_SIZE_32KB = 1 << 13,
    SET_SIZE      = SET_SIZE_32KB,
    SET_INCR      = 1 << 6,
    WAY_INCR      = 1 << 30,
    WAY_SIZE      = 4,
  };
};

//-----------------------------------------------------------------------------
INTERFACE [arm && armca9]:

EXTENSION class Mmu
{
  enum
  {
    SET_SIZE_16KB = 1 << 12,
    SET_SIZE_32KB = 1 << 13,
    SET_SIZE_64KB = 1 << 14,
    SET_SIZE      = SET_SIZE_32KB,
    SET_INCR      = 1 << 5,
    WAY_INCR      = 1 << 30,
    WAY_SIZE      = 4,
  };

};

//-----------------------------------------
IMPLEMENTATION [arm && (armca8 || armca9)]:

PRIVATE
template< unsigned long Flush_area, bool Ram >
template< typename T >
static inline
void Mmu<Flush_area, Ram>::set_way_full_op(T const &f)
{
  unsigned wv, w;
  for (w = 0, wv = 0; w < WAY_SIZE; ++w, wv += WAY_INCR)
    for (unsigned s = 0; s < SET_SIZE; s += SET_INCR)
      f(wv | s);
  btc_inv();
  __asm__ __volatile__ ("dsb; isb");
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_dcache()
{
  __asm__ __volatile__("dsb");
  set_way_full_op(set_way_dcache_flush_op());
}


IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_cache()
{
  __asm__ __volatile__ (
      "    dsb       \n"
      // Invalidate Entire Instruction Cache Register + BTC
      "    mcr p15, 0, r0, c7, c5, 0 \n"
      : : : "memory");

  set_way_full_op(set_way_dcache_flush_op());
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::clean_dcache()
{
  __asm__ __volatile__ (
      "    dsb       \n" // Drain Synchronization Barrier Register
      : : : "memory");

  set_way_full_op(set_way_dcache_clean_op());
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && sa1100]:

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_cache()
{
  register Mword dummy;
  asm volatile (
      "     add %0, %1, #8192           \n" // 8k flush area
      " 1:  ldr r0, [%1], %2            \n" // 32 bytes cache line size
      "     teq %1, %0                  \n"
      "     bne 1b                      \n"
      "     mov r0, #0                  \n"
      "     mcr  p15, 0, r0, c7, c7, 0  \n"
      "     mcr  p15, 0, r0, c7, c10, 4 \n" // drain WB
      : "=r" (dummy)
      : "r" (Flush_area), "i" (Cache_line_size)
      : "r0"
      );
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache()
{
  register Mword dummy;
  asm volatile (
      "     add %0, %1, #8192 \n" // 8k flush area
      " 1:  ldr r0, [%1], %2  \n" // 32 bytes cache line size
      "     teq %1, %0        \n"
      "     bne 1b            \n"
      "     mcr  p15, 0, r0, c7, c10, 4 \n" // drain WB
      : "=r" (dummy)
      : "r" (Flush_area), "i" (Cache_line_size)
      : "r0"
      );
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_dcache()
{
  register Mword dummy;
  asm volatile (
      "     add %0, %1, #8192           \n" // 8k flush area
      " 1:  ldr r0, [%1], %2            \n" // 32 bytes cache line size
      "     teq %1, %0                  \n"
      "     bne 1b                      \n"
      "     mov  r0, #0                 \n"
      "     mcr  p15, 0, r0, c7, c6, 0  \n" // inv D cache
      "     mcr  p15, 0, r0, c7, c10, 4 \n" // drain WB
      : "=r" (dummy)
      : "r" (Flush_area), "i" (Cache_line_size)
      : "r0"
      );

}


//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && pxa]:

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_cache()
{
  register Mword dummy1, dummy2;
  asm volatile
    (
     // write back data cache
     " 1: mcr p15,0,%0,c7,c2,5                           \n\t"
     "    add %0, %0, #32                                \n\t"
     "    subs %1, %1, #1                                \n\t"
     "    bne 1b                                         \n\t"
     // drain write buffer
     "    mcr  p15, 0, %0, c7, c7, 0  \n"
     "    mcr p15, 0, r0, c7, c10, 4                     \n\t"
     :
     "=r" (dummy1),
     "=r" (dummy2)
     :
     "0" (Flush_area),
     "1" (Ram?2048:1024)
    );
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache()
{
  register Mword dummy1, dummy2;
  asm volatile
    (
     // write back data cache
     " 1: mcr p15,0,%0,c7,c2,5                           \n\t"
     "    add %0, %0, #32                                \n\t"
     "    subs %1, %1, #1                                \n\t"
     "    bne 1b                                         \n\t"
     // drain write buffer
     "    mcr p15, 0, r0, c7, c10, 4                     \n\t"
     :
     "=r" (dummy1),
     "=r" (dummy2)
     :
     "0" (Flush_area),
     "1" (Ram?2048:1024)
    );
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_dcache()
{
  register Mword dummy1, dummy2;
  asm volatile
    (
     // write back data cache
     " 1: mcr p15,0,%0,c7,c2,5                           \n\t"
     "    add %0, %0, #32                                \n\t"
     "    subs %1, %1, #1                                \n\t"
     "    bne 1b                                         \n\t"
     "    mcr  p15, 0, %0, c7, c6, 0  \n" // inv D cache
     // drain write buffer
     "    mcr p15, 0, r0, c7, c10, 4                     \n\t"
     :
     "=r" (dummy1),
     "=r" (dummy2)
     :
     "0" (Flush_area),
     "1" (Ram?2048:1024)
    );
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && arm920t]:

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_cache()
{
  Mem::dsb();

  // clean and invalidate dcache
  for (unsigned long index = 0; index < (1 << (32 - 26)); ++index)
    for (unsigned long seg = 0; seg < 256; seg += 32)
      asm volatile("mcr p15,0,%0,c7,c14,2" : : "r" ((index << 26) | seg));

  // invalidate icache
  asm volatile("mcr p15,0,%0,c7,c5,0" : : "r" (0) : "memory");
}


IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache()
{
  flush_cache();
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_dcache()
{
  flush_cache();
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && 926]:

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_cache()
{
  asm volatile
    (
     // write back data cache
     "1:  mrc p15, 0, r15, c7, c14, 3                    \n\t"
     "    bne 1b                                         \n\t"
     // drain write buffer
     "    mcr p15, 0, %0, c7, c7, 0                      \n\t"
     "    mcr p15, 0, %0, c7, c10, 4                     \n\t"
     : :
     "r" (0)
    );
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::clean_dcache()
{
  asm volatile
    (
     // write back data cache
     "1:  mrc p15, 0, r15, c7, c14, 3                    \n\t"
     "    bne 1b                                         \n\t"
     // drain write buffer
     "    mcr p15, 0, %0, c7, c10, 4                     \n\t"
     : :
     "r" (0)
    );
}

IMPLEMENT
template< unsigned long Flush_area, bool Ram >
FIASCO_NOINLINE void Mmu<Flush_area, Ram>::flush_dcache()
{
  asm volatile
    (
     // write back data cache
     "1:  mrc p15, 0, r15, c7, c14, 3                    \n\t"
     "    bne 1b                                         \n\t"
     "    mcr  p15, 0, %0, c7, c6, 0  \n" // inv D cache
     // drain write buffer
     "    mcr p15, 0, %0, c7, c10, 4                     \n\t"
     : :
     "r" (0)
    );
}


