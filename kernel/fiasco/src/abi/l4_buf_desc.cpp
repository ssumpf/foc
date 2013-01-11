INTERFACE:

#include "types.h"

/**
 * Description of the mapping buffer registers contained in the UTCB 
 * (used e.g. during IPC).
 * The utcb can contain buffers that describe memory regions, bits in the
 * I/O bitmap or capabilities. The buffer description is used to find the
 * first buffer for each type.
 * Additionally, the buffer description contains a flag to specify the
 * willingness to receive FPU state in an IPC operation.
 *
 * Note that a single buffer might occupy more than one word in the buffer-
 * registers array in the UTCB. The L4_buf_iter class can be used to iterate
 * over buffers.
 */
class L4_buf_desc
{
public:
  enum Flags
  {
    /**
     * \brief Flag the willingness to receive FPU state during IPC.
     *
     * If this flag is set, the receiving thread in an IPC is willing
     * to receive the status of the floating point unit (FPU) from its partner
     * as part of an IPC. Conceptually, this flag adds the FPU of the
     * receiver as an additional message receiver buffer.
     * The sender must set the corresponding flag L4_msg_tag::Transfer_fpu.
     */
    Inherit_fpu = (1UL << 24)
  };

  /**
   * Create an uninitialized buffer descriptor.
   * \note The value of the buffer descriptor is unpredictable.
   */
  L4_buf_desc() {}

  /**
   * Create a buffer descriptor with given values.
   * \param mem the BR index for the first memory buffer item.
   * \param io the BR index for the first I/O-port buffer item.
   * \param obj the BR index for the first object/capability buffer item.
   * \param flags the flags, such as, Inherit_fpu.
   *
   * The buffer registers must contain blocks of buffers of identical
   * type (memory, caps, I/O-ports) starting at the given index. The first
   * non-matching item terminates the items of the particular type.
   * \see Utcb and L4_msg_tag
   */
  L4_buf_desc(unsigned mem, unsigned io, unsigned obj,
              unsigned flags = 0)
  : _raw(mem | (io << 5) | (obj << 10) | flags)
  {}

  /**
   * Index of the first memory receive buffer.
   * \return the index of the first receive buffer for memory mappings.
   *
   * The memory receive items use two BRs each.
   * \see L4_fpage, L4_msg_item
   */
  unsigned mem() const { return _raw & ((1UL << 5)-1); }

  /**
   * Index of the first I/O-port buffer item.
   * \return the index of the first BR containing a I/O-port buffer.
   *
   * The I/O-port buffer items use two BRs each.
   * \see L4_fpage, L4_msg_item.
   */
  unsigned io()  const { return (_raw >> 5) & ((1UL << 5)-1); }

  /**
   * Index of the first object receive buffer.
   * \return the BR index for the first object/capability receive buffer.
   *
   * An object receive buffer may use one or two BRs depending on the
   * value in the L4_msg_item in the first BR.
   * \see L4_msg_item, L4_fpage.
   */
  unsigned obj() const { return (_raw >> 10) & ((1UL << 5)-1); }

  /**
   * The flags of the BDR.
   * \return flags encoded in the BDR, see #Inherit_fpu, L4_buf_desc::Flags.
   * \note The return value may have reserved bits set.
   */
  Mword flags() const { return _raw; }

  /**
   * Get the raw binary representation of the buffer descriptor.
   * \return binary representation of the buffer descriptor.
   */
  Mword raw() const { return _raw; }

private:
  /**
   * A single machine word that describes the buffers that follow:
   * - Bits 0..4: The index of the first memory buffer.
   * - Bits 5..9: The index of the first io buffer.
   * - Bits 10..14: The index of the first capability buffer.
   * - Bits 15..23: Unused
   * - Bits 24..31: Flags as defined above (only #Inherit_fpu is in use).
   */
  Mword _raw;
};
