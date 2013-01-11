/**
 * \internal
 * \file
 * \brief X86 virtualization interface.
 */
/*
 * (c) 2010 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *          Alexander Warg <warg@os.inf.tu-dresden.de>
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
 * \defgroup l4_vm_vmx_api VM API for VMX
 * \brief Virtual machine API for VMX.
 * \ingroup l4_vm_api
 */


/**
 * \brief Additional VMCS fields.
 * \ingroup l4_vm_vmx_api
 */
enum
{
  L4_VM_VMX_VMCS_CR2              = 0x6830,
};

/**
 * \brief Return length in bytes of a VMCS field.
 * \ingroup l4_vm_vmx_api
 *
 * \param field  Field number.
 * \return Width of field in bytes.
 */
L4_INLINE
unsigned
l4_vm_vmx_field_len(unsigned field);

/**
 * \brief Get pointer into VMCS.
 * \ingroup l4_vm_vmx_api
 *
 * \param vmcs   Pointer to VMCS buffer.
 * \param field  Field number.
 *
 * \param Pointer to field in the VMCS.
 */
L4_INLINE
void *
l4_vm_vmx_field_ptr(void *vmcs, unsigned field);


/* Implementations */

L4_INLINE
unsigned
l4_vm_vmx_field_len(unsigned field)
{
  static const char widths[4] = { 2, 8, 4, sizeof(l4_umword_t) };
  return widths[field >> 13];
}

L4_INLINE
void *
l4_vm_vmx_field_ptr(void *vmcs, unsigned field)
{
  return (void *)((char *)vmcs
                          + ((field >> 13) * 4 + ((field >> 10) & 3) + 1) * 0x80
                          + l4_vm_vmx_field_len(field) * ((field >> 1) & 0xff));
}
