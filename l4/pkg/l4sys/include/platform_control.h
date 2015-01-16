/**
 * \file
 * \brief Platform control object.
 */
/*
 * (c) 2014 Alexander Warg <alexander.warg@kernkonzept.com>
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

#include <l4/sys/types.h>
#include <l4/sys/utcb.h>

/**
 * \defgroup l4_platform_control_api
 * \ingroup  l4_kernel_object_api
 * \brief Class definition for the platform-control object.
 *
 * <c>\#include <l4/sys/platform_control.h><\c>
 *
 */


/**
 * \brief Enter suspend to RAM.
 * \param pfc     Capability selector for the platform-control object
 * \param extras  some extra platform-specific information needed to enter
 *                suspend to RAM.
 * \return Syscall return tag
 */
L4_INLINE l4_msgtag_t
l4_platform_ctl_system_suspend(l4_cap_idx_t pfc,
                               l4_umword_t extras) L4_NOTHROW;

/**
 * \internal
 */
L4_INLINE l4_msgtag_t
l4_platform_ctl_system_suspend_u(l4_cap_idx_t pfc,
                                 l4_umword_t extras,
                                 l4_utcb_t *utcb) L4_NOTHROW;


/**
 * \brief Reboot the system.
 * \param pfc     Capability selector for the platform-control object
 * \return Syscall return tag
 */
L4_INLINE l4_msgtag_t
l4_platform_ctl_system_reboot(l4_cap_idx_t pfc) L4_NOTHROW;

/**
 * \brief Shutdown the system.
 * \param pfc     Capability selector for the platform-control object
 * \return Syscall return tag
 */
L4_INLINE l4_msgtag_t
l4_platform_ctl_system_shutdown(l4_cap_idx_t pfc) L4_NOTHROW;

/**
 * \internal
 */
L4_INLINE l4_msgtag_t
l4_platform_ctl_system_shutdown_u(l4_cap_idx_t pfc,
                                  l4_umword_t reboot,
                                  l4_utcb_t *utcb) L4_NOTHROW;


/**
 * \internal
 * \brief Operations on the platform-control objects.
 * \ingroup l4_platform_control_api
 */
enum L4_platform_ctl_ops
{
  L4_PLATFORM_CTL_SYS_SUSPEND_OP  = 0UL, /**< Suspend */
  L4_PLATFORM_CTL_SYS_SHUTDOWN_OP = 1UL, /**< shutdown/reboot */
};

enum L4_platformctl_proto
{
  L4_PROTO_PLATFORM_CTL = 0 /**< platform-control protocol */
};


/* IMPLEMENTATION -----------------------------------------------------------*/

#include <l4/sys/ipc.h>

L4_INLINE l4_msgtag_t
l4_platform_ctl_system_suspend_u(l4_cap_idx_t pfc,
                                 l4_umword_t extras,
                                 l4_utcb_t *utcb) L4_NOTHROW
{
  l4_msg_regs_t *v = l4_utcb_mr_u(utcb);
  v->mr[0] = L4_PLATFORM_CTL_SYS_SUSPEND_OP;
  v->mr[1] = extras;
  return l4_ipc_call(pfc, utcb, l4_msgtag(L4_PROTO_PLATFORM_CTL, 2, 0, 0),
                                          L4_IPC_NEVER);
}


L4_INLINE l4_msgtag_t
l4_platform_ctl_system_shutdown_u(l4_cap_idx_t pfc,
                                  l4_umword_t reboot,
                                  l4_utcb_t *utcb) L4_NOTHROW
{
  l4_msg_regs_t *v = l4_utcb_mr_u(utcb);
  v->mr[0] = L4_PLATFORM_CTL_SYS_SHUTDOWN_OP;
  v->mr[1] = reboot;
  return l4_ipc_call(pfc, utcb, l4_msgtag(L4_PROTO_PLATFORM_CTL, 2, 0, 0),
                                          L4_IPC_NEVER);
}


L4_INLINE l4_msgtag_t
l4_platform_ctl_system_suspend(l4_cap_idx_t pfc,
                               l4_umword_t extras) L4_NOTHROW
{
  return l4_platform_ctl_system_suspend_u(pfc, extras, l4_utcb());
}

L4_INLINE l4_msgtag_t
l4_platform_ctl_system_reboot(l4_cap_idx_t pfc) L4_NOTHROW
{
  return l4_platform_ctl_system_shutdown_u(pfc, 1, l4_utcb());
}


L4_INLINE l4_msgtag_t
l4_platform_ctl_system_shutdown(l4_cap_idx_t pfc) L4_NOTHROW
{
  return l4_platform_ctl_system_shutdown_u(pfc, 0, l4_utcb());
}

