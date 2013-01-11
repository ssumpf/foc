INTERFACE:

#include "types.h"

/**
 * A L4 flex page.
 *
 * A flex page represents a naturally aligned area of mappable space,
 * such as memory, I/O-ports, and capabilities (kernel objects).
 * There is also a representation for describing a flex page that represents
 * the whole of all these address spaces. The size of a flex page is given
 * as a power of two.
 *
 *
 * The internal representation is a single machine word with the following
 * layout:
 *   \verbatim
 *   +- bitsize-12 +- 11-6 -+ 5-4 -+-- 3-0 -+
 *   | page number | order  | type | rights |
 *   +-------------+--------+------+--------+
 *   \endverbatim
 *
 *   - The rights bits (0-3) denote the access rights to an object, see
 *     L4_fpage::Rights.
 *   - The \a type of a flex page is denotes the address space that is
 *     referenced by that flex page (see L4_fpage::Type).
 *   - The order is the exponent for the size calculation (size = 2^order).
 *   - The page number denotes the page address within the address space
 *     denoted by \a type. For example when \a type is #Memory, the \a page
 *     \a number must contain the most significant bits of a virtual address
 *     that must be aligned to \a order bits.  In the case that \a type
 *     equals either #Io or #Obj, the \a page \a number contains all bits of
 *     the I/O-port number or the capability index, respectively (note, the
 *     values must also be aligned according to the value of \a order).
 *
 */
class L4_fpage
{
public:
  /**
   * Data type to represent the binary representation of a flex page.
   */
  typedef Mword Raw;

  /**
   * Address space type of a flex page.
   */
  enum Type
  {
    Special = 0, ///< Special flex pages, either invalid or all spaces.
    Memory,      ///< Memory flex page
    Io,          ///< I/O-port flex page
    Obj          ///< Object flex page (capabilities)
  };

  enum { Addr_shift = 12 };

private:
  /**
   * Create a flex page with the given parameters.
   */
  L4_fpage(Type type, Mword address, unsigned char order,
           unsigned char rights)
  : _raw(address | Raw(rights) | (Raw(order) << 6) | (Raw(type) << 4))
  {}

public:
  enum
  {
    Whole_space = 63 ///< Value to use as \a order for a whole address space.
  };

  /**
   * Create an I/O flex page.
   *
   * IO flex pages do not support access rights other than RW or nothing.
   * \param port base I/O-port number (0..65535), must be aligned to
   *             2^\a order. The value is shifted by #Addr_shift bits to the
   *             left.
   * \param order the order of the I/O flex page, size is 2^\a order ports.
   */
  static L4_fpage io(Mword port, unsigned char order)
  { return L4_fpage(Io, port << Addr_shift, order, 0); }

  /**
   * Create an object flex page.
   *
   * \param idx capability index, note capability indexes are multiples of
   *            0x1000. (hence \a idx is not shifted)
   * \param order The size of the flex page is 2^\a order. The value in \a idx
   *              must be aligned to 2^(\a order + #Addr_shift.
   */
  static L4_fpage obj(Mword idx, unsigned char order, unsigned char rights = 0)
  { return L4_fpage(Obj, idx & (~0UL << Addr_shift), order, rights); }

  /**
   * Create a memory flex page.
   *
   * \param addr The virtual address. Only the most significant bits are
   *             considered, bits from 0 to \a order-1 are dropped.
   * \param order The size of the flex page is 2^\a order in bytes.
   */
  static L4_fpage mem(Mword addr, unsigned char order, unsigned char rights = 0)
  { return L4_fpage(Memory, addr & (~0UL << Addr_shift), order, rights); }

  /**
   * Create a nil (invalid) flex page.
   */
  static L4_fpage nil()
  { return L4_fpage(0); }

  /**
   * Create a special receive flex page representing
   * all available address spaces at once. This is used
   * for page-fault and exception IPC.
   */
  static L4_fpage all_spaces(unsigned char rights = 0)
  { return L4_fpage(Special, 0, Whole_space, rights); }

  /**
   * Create a flex page from the raw value.
   */
  explicit L4_fpage(Raw raw) : _raw(raw) {}

  /**
   * Get the type, see #L4_fpage::Type.
   * \return the type of the flex page.
   */
  Type type() const { return Type((_raw >> 4) & 3); }

  /**
   * Get the order of a flex page.
   * \return the order of the flex page (size is 2^\a order).
   */
  unsigned char order() const { return (_raw >> 6) & 0x3f; }

  /**
   * The robust type for carrying virtual memory addresses.
   */
  typedef Virt_addr Mem_addr;

  /**
   * Get the virtual address of a memory flex page.
   *
   * \pre type() must return #Memory to return a valid value.
   * \return The virtual memory base address of the flex page.
   */
  Virt_addr mem_address() const
  { return Virt_addr(_raw & (~0UL << Addr_shift)); }

  /**
   * Get the capability address of an object flex page.
   *
   * \pre type() must return #Obj to return a valid value.
   * \return The capability value (index) of this flex page.
   *         This value is not shifted, so it is a multiple of 0x1000.
   *         See obj_index() for reference.
   */
  Mword obj_address() const { return _raw & (~0UL << Addr_shift); }

  /**
   * Get the I/O-port number of an I/O flex page.
   * \pre type() must return #Io to return a valid value.
   * \return The I/O-port index of this flex page.
   */
  Mword io_address() const { return _raw >> Addr_shift; }

  /**
   * Get the capability index of an object flex page.
   *
   * \pre type() must return #Obj to return a valid value.
   * \return The index into the capability table provided by this flex page.
   *         This value is shifted #Addr_shift to be a real index
   *         (opposed to obj_address()).
   */
  Mword obj_index() const { return _raw >> Addr_shift; }

  /**
   * Test for memory flex page (if type() is #Memory).
   * \return true if type() is #Memory.
   */
  bool is_mempage() const { return type() == Memory; }

  /**
   * Test for I/O flex page (if type() is #Io).
   * \return true if type() is #Io.
   */
  bool is_iopage() const { return type() == Io; }

  /**
   * Test for object flex page (if type() is #Obj).
   * \return true if type() is #Obj.
   */
  bool is_objpage() const { return type() == Obj; }


  /**
   * Is the flex page the whole address space?
   * @return not zero, if the flex page covers the
   *   whole address space.
   */
  Mword is_all_spaces() const { return (_raw & 0xff8) == (Whole_space << 6); }

  /**
   * Is the flex page valid?
   * \return not zero if the flex page
   *    contains a value other than 0.
   */
  Mword is_valid() const { return _raw; }

  /**
   * Get the binary representation of the flex page.
   * \return this flex page in binary representation.
   */
  Raw raw() const { return _raw; }

private:
  Raw _raw;

public:

  /**
   * Rights bits for flex pages.
   *
   * The particular semantics of the rights bits in a flex page differ depending on the
   * type of the flex page. For memory there are #R, #W, and #X rights. For
   * I/O-ports there must be #R and #W, to get access. For object (capabilities)
   * there are #CD, #CR, #CS, and #CW rights on the object and additional
   * rights in the map control value of the map operation (see L4_fpage::Obj_map_ctl).
   */
  enum Rights
  {
    R   = 4, ///< Memory flex page is readable
    W   = 2, ///< Memory flex page is writable
    X   = 1, ///< Memory flex page is executable (often equal to #R)

    RX  = R | X,     ///< Memory flex page is readable and executable
    RWX = R | W | X, ///< Memory flex page is readable, writeable, and executable
    RW  = R | W,     ///< Memory flex page is readable and writable
    WX  = W | X,     ///< Memory flex page is writable and executable


    CD    = 0x8, ///< Object flex page: delete rights
    CR    = 0x4, ///< Object flex page: read rights (w/o this the mapping is not present)
    CS    = 0x2, ///< Object flex page: strong semantics (object specific, i.e. not having
                 ///  this right on an IPC gate demotes all capabilities transferred via this
                 ///  IPC gate to also suffer this right.
    CW    = 0x1, ///< Object flex page: write rights (purely object specific)

    CRW   = CR | CW,       ///< Object flex page: combine #CR and #CW
    CRS   = CR | CS,       ///< Object flex page: combine #CR and #CS
    CRWS  = CRW | CS,      ///< Object flex page: combine #CR, #CW, and #CS
    CWS   = CW | CS,       ///< Object flex page: combine #CS and #CW
    CWSD  = CW | CS | CD,  ///< Object flex page: combine #CS, #CW, and #CD
    CRWSD = CRWS | CD,     ///< Object flex page: combine #CR, #CW, #CS, and #CD

    FULL  = 0xf, ///< All rights shall be transferred, independent of the type
  };

  /**
   * Get the rights associated with this flexpage.
   * \return The rights associated with this flex page. The semantics of this
   *         value also depends on the type (type()) of the flex page.
   */
  Rights rights() const { return Rights(_raw & FULL); }

  /**
   * Remove the given rights from this flex page.
   * \param r the rights to remove. The semantics depend on the
   *          type (type()) of the flex page.
   */
  void mask_rights(Rights r) { _raw &= (Mword(r) | ~0x0fUL); }
};

