/**
 * \file
 * \brief Kernel Info Page access functions.
 * \ingroup l4_api
 */
/*
 * (c) 2008-2013 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>
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

#include <l4/sys/compiler.h>
#include <l4/sys/l4int.h>

#include <l4/sys/__kip-arch.h>

/**
 * \internal
 */
struct l4_kip_platform_info
{
  char                             name[16];
  l4_uint32_t                      is_mp;
  struct l4_kip_platform_info_arch arch;
};

#if L4_MWORD_BITS == 32
#  include <l4/sys/__kip-32bit.h>
#else
#  include <l4/sys/__kip-64bit.h>
#endif

/**
 * \addtogroup l4_kip_api
 *
 * C interface for the Kernel Interface Page:<br>
 * <c>\#include <l4/sys/kip.h></c>
 */
/*@{*/

/**
 * \internal
 */
enum l4_kernel_info_consts_t
{
  L4_KIP_VERSION_FIASCO      = 0x87004444,
  L4_KIP_VERSION_FIASCO_MASK = 0xff00ffff,
};

/**
 * \brief Kernel Info Page identifier ("L4µK").
 */
#define L4_KERNEL_INFO_MAGIC (0x4BE6344CL) /* "L4µK" */


/**
 * \brief  Get the kernel version.
 * \param kip Kernel Interface Page.
 * \return Kernel version string. 0 if KIP could not be mapped.
 */
L4_INLINE l4_umword_t l4_kip_version(l4_kernel_info_t *kip) L4_NOTHROW;

/**
 * \brief  Get the kernel version string.
 * \param kip Kernel Interface Page.
 * \return Kernel version string.
 */
L4_INLINE const char *l4_kip_version_string(l4_kernel_info_t *kip) L4_NOTHROW;

/**
 * \brief Return offset in bytes of version_strings relative to the KIP base.
 *
 * \param kip	Pointer to the kernel into page (KIP).
 *
 * \return offset of version_strings relative to the KIP base address, in
 *         bytes.
 */
L4_INLINE int
l4_kernel_info_version_offset(l4_kernel_info_t *kip) L4_NOTHROW;

/*@}*/

/*************************************************************************
 * Implementations
 *************************************************************************/

L4_INLINE l4_umword_t
l4_kip_version(l4_kernel_info_t *kip) L4_NOTHROW
{ return kip->version & L4_KIP_VERSION_FIASCO_MASK; }

L4_INLINE const char*
l4_kip_version_string(l4_kernel_info_t *k) L4_NOTHROW
{ return (const char *)k + l4_kernel_info_version_offset(k); }

L4_INLINE int
l4_kernel_info_version_offset(l4_kernel_info_t *kip) L4_NOTHROW
{ return kip->offset_version_strings << 4; }
