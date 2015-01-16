/**
 * \internal
 * \file
 * \brief   Common flex-page definitions.
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Björn Döbel <doebel@os.inf.tu-dresden.de>,
 *               Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 */
#pragma once

/**
 * \defgroup l4_fpage_api Flex pages
 * \ingroup l4_api
 * \brief Flex-page related API.
 *
 * A flex page is a page with a variable size, that can describe memory,
 * IO-Ports (IA32 only), and sets of kernel objects.
 *
 * A flex page describes an always size aligned region of an address space.
 * The size is given in a log2 scale. This means the size in elements (bytes
 * for memory, ports for IO-Ports, and capabilities for kernel objects) is
 * always a power of two.
 *
 * A flex page also carries type and access right information for the
 * described region. The type information selects the address space in which
 * the flex page is valid. Access rights have a meaning depending on the
 * specific address space (type).
 *
 * There exists a special type for defining \em receive \em windows or for
 * the l4_task_unmap() method, that can be used to describe all address
 * spaces (all types) with a single flex page.
 */

/**
 * \brief L4 flexpage structure
 * \ingroup l4_fpage_api
 */
enum l4_fpage_consts
{
  L4_FPAGE_RIGHTS_SHIFT = 0,  ///< Access permissions shift
  L4_FPAGE_TYPE_SHIFT   = 4,  ///< Flexpage type shift (memory, IO port, obj...)
  L4_FPAGE_SIZE_SHIFT   = 6,  ///< Flexpage size shift (log2-based)
  L4_FPAGE_ADDR_SHIFT   = 12, ///< Page address shift

  L4_FPAGE_RIGHTS_BITS = 4,   ///< Access permissions size
  L4_FPAGE_TYPE_BITS   = 2,   ///< Flexpage type size (memory, IO port, obj...)
  L4_FPAGE_SIZE_BITS   = 6,   ///< Flexpage size size (log2-based)
  L4_FPAGE_ADDR_BITS   = L4_MWORD_BITS - L4_FPAGE_ADDR_SHIFT,  ///< Page address size


  L4_FPAGE_RIGHTS_MASK  = ((1UL << L4_FPAGE_RIGHTS_BITS) - 1) << L4_FPAGE_RIGHTS_SHIFT,
  L4_FPAGE_TYPE_MASK    = ((1UL << L4_FPAGE_TYPE_BITS)   - 1) << L4_FPAGE_TYPE_SHIFT,
  L4_FPAGE_SIZE_MASK    = ((1UL << L4_FPAGE_SIZE_BITS)   - 1) << L4_FPAGE_SIZE_SHIFT,
  L4_FPAGE_ADDR_MASK    = ~0UL << L4_FPAGE_ADDR_SHIFT,
};

/**
 * \brief L4 flexpage type
 * \ingroup l4_fpage_api
 */
typedef union {
  l4_umword_t fpage;          ///< Raw value
  l4_umword_t raw;            ///< Raw value
} l4_fpage_t;

/** \brief Constants for flexpages
 * \ingroup l4_fpage_api
 */
enum
{
  L4_WHOLE_ADDRESS_SPACE = 63 /**< Whole address space size */
};

/**
 * \brief Send-flex-page types
 * \ingroup l4_fpage_api
 */
typedef struct {
  l4_umword_t snd_base;      ///< Offset in receive window (send base)
  l4_fpage_t fpage;          ///< Source flex-page descriptor
} l4_snd_fpage_t;


/** \brief Flex-page rights
 * \ingroup l4_fpage_api
 */
enum L4_fpage_rights
{
  L4_FPAGE_RO    = 4, /**< Read-only flex page  */
  L4_FPAGE_RW    = 6, /**< Read-write flex page */
  L4_FPAGE_RX    = 5,
  L4_FPAGE_RWX   = 7,
  L4_FPAGE_X     = 1,
  L4_FPAGE_W     = 2,
};

/** \brief Cap-flex-page rights
 * \ingroup l4_fpage_api
 */
enum L4_cap_fpage_rights
{
  L4_CAP_FPAGE_R     = 0x4, /**< Read-only cap  */
  L4_CAP_FPAGE_RO    = 0x4, /**< Read-only cap  */
  L4_CAP_FPAGE_RW    = 0x5, /**< Read-write cap */
  L4_CAP_FPAGE_RS    = 0x6,
  L4_CAP_FPAGE_RWS   = 0x7,
  L4_CAP_FPAGE_S     = 0x2,
  L4_CAP_FPAGE_W     = 0x1,
  L4_CAP_FPAGE_D     = 0x8,
  L4_CAP_FPAGE_RWSD  = 0xf,
  L4_CAP_FPAGE_RSD   = 0xe,
};

/** \brief Flex-page type
 * \ingroup l4_fpage_api
 */
enum L4_fpage_type
{
  L4_FPAGE_SPECIAL = 0,
  L4_FPAGE_MEMORY  = 1,
  L4_FPAGE_IO      = 2,
  L4_FPAGE_OBJ     = 3,
};

/** \brief Flex-page map control flags
 * \ingroup l4_fpage_api
 */
enum L4_fpage_control
{
  L4_FPAGE_CONTROL_OFFSET_SHIFT = 10,
  L4_FPAGE_CONTROL_MASK = ~0UL << L4_FPAGE_CONTROL_OFFSET_SHIFT,
};

/** \brief Flex-page map control for capabilities (snd_base)
 * \ingroup l4_fpage_api
 */
enum L4_obj_fpage_ctl
{
  L4_FPAGE_C_NO_REF_CNT = 0x10,
  L4_FPAGE_C_REF_CNT    = 0x00,
};


/** \brief Flex-page cacheability option
 * \ingroup l4_fpage_api
 */
enum l4_fpage_cacheability_opt_t
{
  /** Enable the cacheability option in a send flex page. */
  L4_FPAGE_CACHE_OPT   = 0x1,

  /** Cacheability option to enable caches for the mapping. */
  L4_FPAGE_CACHEABLE   = 0x3,

  /** Cacheability option to enable buffered writes for the mapping. */
  L4_FPAGE_BUFFERABLE  = 0x5,

  /** Cacheability option to disable caching for the mapping. */
  L4_FPAGE_UNCACHEABLE = 0x1
};


/** \brief Special constants for IO flex pages
 * \ingroup l4_fpage_api
 */
enum
{
  /** Whole I/O address space size */
  L4_WHOLE_IOADDRESS_SPACE  = 16,

  /** Maximum I/O port address */
  L4_IOPORT_MAX             = (1L << L4_WHOLE_IOADDRESS_SPACE)
};



/**
 * \brief   Create a memory flex page.
 * \ingroup l4_fpage_api
 *
 * \param   address      Flex-page start address
 * \param   size         Flex-page size (log2), #L4_WHOLE_ADDRESS_SPACE to
 *                       specify the whole address space (with \a address 0)
 * \param   rights       Access rights, see #l4_fpage_rights
 * \return  Memory flex page
 */
L4_INLINE l4_fpage_t
l4_fpage(unsigned long address, unsigned int size, unsigned char rights) L4_NOTHROW;

/**
 * \brief   Get a flex page, describing all address spaces at once.
 * \ingroup l4_fpage_api
 *
 * \return  Special \em all-spaces flex page.
 */
L4_INLINE l4_fpage_t
l4_fpage_all(void) L4_NOTHROW;

/**
 * \brief   Get an invalid flex page.
 * \ingroup l4_fpage_api
 *
 * \return  Special \em invalid flex page.
 */
L4_INLINE l4_fpage_t
l4_fpage_invalid(void) L4_NOTHROW;


/**
 * \brief   Create an IO-port flex page.
 * \ingroup l4_fpage_api
 *
 * \param   port         I/O-flex-page port base
 * \param   size         I/O-flex-page size, #L4_WHOLE_IOADDRESS_SPACE to
 *                       specify the whole I/O address space (with \a port 0)
 * \return  I/O flex page
 */
L4_INLINE l4_fpage_t
l4_iofpage(unsigned long port, unsigned int size) L4_NOTHROW;


/**
 * \brief   Create a kernel-object flex page.
 * \ingroup l4_fpage_api
 *
 * \param   obj       Base capability selector.
 * \param   order     Log2 size (number of capabilities).
 * \param   rights    Access rights
 * \return  Flex page for a set of kernel objects.
 *
 * \todo What are the possible values for the rights parameter?
 */
L4_INLINE l4_fpage_t
l4_obj_fpage(l4_cap_idx_t obj, unsigned int order, unsigned char rights) L4_NOTHROW;

/**
 * \brief Test if the flex page is writable.
 * \ingroup l4_fpage_api
 *
 * \param   fp  Flex page.
 * \return  != 0 if flex page is writable, 0 if not
 */
L4_INLINE int
l4_is_fpage_writable(l4_fpage_t fp) L4_NOTHROW;


/**
 * \defgroup l4_msgitem_api Message Items
 * \ingroup l4_ipc_api
 * \brief Message item related functions.
 *
 * Message items are typed items that can be transferred via IPC
 * operations. Message items are also used to specify receive windows for
 * typed items to be received.
 * Message items are placed in the message registers (MRs) of the UTCB of
 * the sending thread.
 * Receive items are placed in the buffer registers (BRs) of the UTCB
 * of the receiving thread.
 *
 * Message items are usually two-word data structures. The first 
 * word denotes the type of the message item (for example a memory flex-page,
 * io flex-page or object flex-page) and the second word contains
 * information depending on the type. There is actually one exception that is
 * a small (one word) receive buffer item for a single capability.
 */

/**
 * \brief Create the first word for a map item for the memory space.
 * \ingroup l4_msgitem_api
 *
 * \param spot  Hot spot address, used to determine what is actually mapped
 *              when send and receive flex page have differing sizes.
 * \param cache Cacheability hints for memory flex pages. See
 *               \link l4_fpage_api::l4_fpage_cacheability_opt_t
 *               Cacheability options \endlink
 * \param grant Indicates if it is a map or a grant item.
 *
 * \return The value to be used as first word in a map item for memory.
 */
L4_INLINE l4_umword_t
l4_map_control(l4_umword_t spot, unsigned char cache, unsigned grant) L4_NOTHROW;

/**
 * \brief Create the first word for a map item for the object space.
 * \ingroup l4_msgitem_api
 *
 * \param   spot  Hot spot address, used to determine what is actually mapped
 *                when send and receive flex pages have different size.
 * \param   grant Indicates if it is a map item or a grant item.
 *
 * \return The value to be used as first word in a map item for kernel objects
 *         or IO-ports.
 */
L4_INLINE l4_umword_t
l4_map_obj_control(l4_umword_t spot, unsigned grant) L4_NOTHROW;

/**
 * \brief Return rights from a flex page.
 * \ingroup l4_fpage_api
 *
 * \param f  Flex page
 * \return Size part of the given flex page.
 */
L4_INLINE unsigned
l4_fpage_rights(l4_fpage_t f) L4_NOTHROW;

/**
 * \brief Return type from a flex page.
 * \ingroup l4_fpage_api
 *
 * \param f  Flex page
 * \return Type part of the given flex page.
 */
L4_INLINE unsigned
l4_fpage_type(l4_fpage_t f) L4_NOTHROW;

/**
 * \brief Return size from a flex page.
 * \ingroup l4_fpage_api
 *
 * \param f  Flex page
 * \return Size part of the given flex page.
 */
L4_INLINE unsigned
l4_fpage_size(l4_fpage_t f) L4_NOTHROW;

/**
 * \brief Return page from a flex page.
 * \ingroup l4_fpage_api
 *
 * \param f  Flex page
 * \return Page part of the given flex page.
 */
L4_INLINE unsigned long
l4_fpage_page(l4_fpage_t f) L4_NOTHROW;

/**
 * \brief Set new right in a flex page.
 * \ingroup l4_fpage_api
 *
 * \param  src         Flex page
 * \param  new_rights  New rights
 *
 * \return Modified flex page with new rights.
 */
L4_INLINE l4_fpage_t
l4_fpage_set_rights(l4_fpage_t src, unsigned char new_rights) L4_NOTHROW;

/**
 * \brief Test whether a given range is completely within an fpage.
 * \ingroup l4_fpage_api
 *
 * \param   fpage    Flex page
 * \param   addr     Address
 * \param   size     Size of range in log2.
 */
L4_INLINE int
l4_fpage_contains(l4_fpage_t fpage, l4_addr_t addr, unsigned size) L4_NOTHROW;

/**
 * \brief Determine maximum flex page size of a region.
 * \ingroup l4_fpage_api
 *
 * \param order    Order value to start with (e.g. for memory
 *                 L4_LOG2_PAGESIZE would be used)
 * \param addr     Address to be covered by the flex page.
 * \param min_addr Start of region / minimal address (including).
 * \param max_addr End of region / maximal address (excluding).
 * \param hotspot  (Optional) hot spot.
 *
 * \return Maximum order (log2-size) possible.
 *
 * \note The start address of the flex-page can be determined with
 *       l4_trunc_size(addr, returnvalue)
 */
L4_INLINE unsigned char
l4_fpage_max_order(unsigned char order, l4_addr_t addr,
                   l4_addr_t min_addr, l4_addr_t max_addr,
                   l4_addr_t hotspot L4_DEFAULT_PARAM(0));

/*************************************************************************
 * Implementations
 *************************************************************************/

L4_INLINE unsigned
l4_fpage_rights(l4_fpage_t f) L4_NOTHROW
{
  return (f.raw & L4_FPAGE_RIGHTS_MASK) >> L4_FPAGE_RIGHTS_SHIFT;
}

L4_INLINE unsigned
l4_fpage_type(l4_fpage_t f) L4_NOTHROW
{
  return (f.raw & L4_FPAGE_TYPE_MASK) >> L4_FPAGE_TYPE_SHIFT;
}

L4_INLINE unsigned
l4_fpage_size(l4_fpage_t f) L4_NOTHROW
{
  return (f.raw & L4_FPAGE_SIZE_MASK) >> L4_FPAGE_SIZE_SHIFT;
}

L4_INLINE unsigned long
l4_fpage_page(l4_fpage_t f) L4_NOTHROW
{
  return (f.raw & L4_FPAGE_ADDR_MASK) >> L4_FPAGE_ADDR_SHIFT;
}

/** \internal */
L4_INLINE l4_fpage_t
__l4_fpage_generic(unsigned long address, unsigned int type,
                   unsigned int size, unsigned char rights) L4_NOTHROW;

L4_INLINE l4_fpage_t
__l4_fpage_generic(unsigned long address, unsigned int type,
                   unsigned int size, unsigned char rights) L4_NOTHROW
{
  l4_fpage_t t;
  t.raw =   ((rights  << L4_FPAGE_RIGHTS_SHIFT) & L4_FPAGE_RIGHTS_MASK)
          | ((type    << L4_FPAGE_TYPE_SHIFT)   & L4_FPAGE_TYPE_MASK)
	  | ((size    << L4_FPAGE_SIZE_SHIFT)   & L4_FPAGE_SIZE_MASK)
	  | ((address                       )   & L4_FPAGE_ADDR_MASK);
  return t;
}

L4_INLINE l4_fpage_t
l4_fpage_set_rights(l4_fpage_t src, unsigned char new_rights) L4_NOTHROW
{
  l4_fpage_t f;
  f.raw = ((L4_FPAGE_TYPE_MASK | L4_FPAGE_SIZE_MASK | L4_FPAGE_ADDR_MASK) & src.raw)
          | ((new_rights << L4_FPAGE_RIGHTS_SHIFT) & L4_FPAGE_RIGHTS_MASK);
  return f;
}

L4_INLINE l4_fpage_t
l4_fpage(unsigned long address, unsigned int size, unsigned char rights) L4_NOTHROW
{
  return __l4_fpage_generic(address, L4_FPAGE_MEMORY, size, rights);
}

L4_INLINE l4_fpage_t
l4_iofpage(unsigned long port, unsigned int size) L4_NOTHROW
{
  return __l4_fpage_generic(port << L4_FPAGE_ADDR_SHIFT, L4_FPAGE_IO, size, L4_FPAGE_RW);
}

L4_INLINE l4_fpage_t
l4_obj_fpage(l4_cap_idx_t obj, unsigned int order, unsigned char rights) L4_NOTHROW
{
  return __l4_fpage_generic(obj, L4_FPAGE_OBJ, order, rights);
}

L4_INLINE l4_fpage_t
l4_fpage_all(void) L4_NOTHROW
{
  return __l4_fpage_generic(0, 0, L4_WHOLE_ADDRESS_SPACE, 0);
}

L4_INLINE l4_fpage_t
l4_fpage_invalid(void) L4_NOTHROW
{
  return __l4_fpage_generic(0, 0, 0, 0);
}


L4_INLINE int
l4_is_fpage_writable(l4_fpage_t fp) L4_NOTHROW
{
  return l4_fpage_rights(fp) & L4_FPAGE_W;
}

L4_INLINE l4_umword_t
l4_map_control(l4_umword_t snd_base, unsigned char cache, unsigned grant) L4_NOTHROW
{
  return (snd_base & L4_FPAGE_CONTROL_MASK)
         | ((l4_umword_t)cache << 4) | L4_ITEM_MAP | grant;
}

L4_INLINE l4_umword_t
l4_map_obj_control(l4_umword_t snd_base, unsigned grant) L4_NOTHROW
{
  return l4_map_control(snd_base, 0, grant);
}

L4_INLINE int
l4_fpage_contains(l4_fpage_t fpage, l4_addr_t addr, unsigned log2size) L4_NOTHROW
{
  l4_addr_t fa = l4_fpage_page(fpage) << L4_FPAGE_ADDR_SHIFT;
  return (fa <= addr)
         && (fa + (1UL << l4_fpage_size(fpage)) >= addr + (1UL << log2size));
}

L4_INLINE unsigned char
l4_fpage_max_order(unsigned char order, l4_addr_t addr,
                   l4_addr_t min_addr, l4_addr_t max_addr,
                   l4_addr_t hotspot)
{
  while (order < 30 /* limit to 1GB flexpages */)
    {
      l4_addr_t mask;
      l4_addr_t base = l4_trunc_size(addr, order + 1);
      if (base < min_addr)
        return order;

      if (base + (1UL << (order + 1)) - 1 > max_addr - 1)
        return order;

      mask = ~(~0UL << (order + 1));
      if (hotspot == ~0UL || ((addr ^ hotspot) & mask))
        break;

      ++order;
    }

  return order;
}
