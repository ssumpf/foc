/**
 * \file
 * \brief IPC-Gate interface.
 */
/*
 * (c) 2009-2010 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
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

#include <l4/sys/utcb.h>
#include <l4/sys/types.h>

/**
 * \brief Bind the IPC-gate to the thread.
 * \ingroup l4_kernel_object_gate_api
 *
 * \param t     Thread to bind the IPC-gate to
 * \param label Label to use
 * \param utcb  UTCB to use.
 *
 * \return System call return tag.
 */
L4_INLINE l4_msgtag_t
l4_ipc_gate_bind_thread(l4_cap_idx_t gate, l4_cap_idx_t thread,
                        l4_umword_t label);

/**
 * \internal
 * \ingroup l4_kernel_object_gate_api
 */
L4_INLINE l4_msgtag_t
l4_ipc_gate_bind_thread_u(l4_cap_idx_t gate, l4_cap_idx_t thread,
                          l4_umword_t label, l4_utcb_t *utcb);

/**
 * \brief Get information on the IPC-gate.
 * \ingroup l4_kernel_object_gate_api
 *
 * \retval label   Label of the gate.
 * \param  utcb    UTCb to use.
 *
 * \return System call return tag.
 */
L4_INLINE l4_msgtag_t
l4_ipc_gate_get_infos(l4_cap_idx_t gate, l4_umword_t *label);

/**
 * \internal
 * \ingroup l4_kernel_object_gate_api
 */
L4_INLINE l4_msgtag_t
l4_ipc_gate_get_infos_u(l4_cap_idx_t gate, l4_umword_t *label, l4_utcb_t *utcb);

/**
 * \brief Operations on the IPC-gate.
 * \ingroup l4_kernel_object_gate_api
 * \hideinitializer
 * \internal
 */
enum L4_ipc_gate_ops
{
  L4_IPC_GATE_BIND_OP     = 0x10, /**< Bind operation */
  L4_IPC_GATE_GET_INFO_OP = 0x11, /**< Info operation */
};


/* IMPLEMENTATION -----------------------------------------------------------*/

#include <l4/sys/ipc.h>

L4_INLINE l4_msgtag_t
l4_ipc_gate_bind_thread_u(l4_cap_idx_t gate,
                          l4_cap_idx_t thread, l4_umword_t label,
                          l4_utcb_t *utcb)
{
  l4_msg_regs_t *m = l4_utcb_mr_u(utcb);
  m->mr[0] = L4_IPC_GATE_BIND_OP;
  m->mr[1] = label;
  m->mr[2] = l4_map_obj_control(0, 0);
  m->mr[3] = l4_obj_fpage(thread, 0, L4_FPAGE_RWX).raw;
  return l4_ipc_call(gate, utcb, l4_msgtag(L4_PROTO_KOBJECT, 2, 1, 0),
                     L4_IPC_NEVER);
}

L4_INLINE l4_msgtag_t
l4_ipc_gate_get_infos_u(l4_cap_idx_t gate, l4_umword_t *label, l4_utcb_t *utcb)
{
  l4_msgtag_t tag;
  l4_msg_regs_t *m = l4_utcb_mr_u(utcb);
  m->mr[0] = L4_IPC_GATE_GET_INFO_OP;
  tag = l4_ipc_call(gate, utcb, l4_msgtag(L4_PROTO_KOBJECT, 1, 0, 0),
                    L4_IPC_NEVER);
  if (!l4_msgtag_has_error(tag) && l4_msgtag_label(tag) >= 0)
    *label = m->mr[0];

  return tag;
}



L4_INLINE l4_msgtag_t
l4_ipc_gate_bind_thread(l4_cap_idx_t gate, l4_cap_idx_t thread,
                        l4_umword_t label)
{
  return l4_ipc_gate_bind_thread_u(gate, thread, label, l4_utcb());
}

L4_INLINE l4_msgtag_t
l4_ipc_gate_get_infos(l4_cap_idx_t gate, l4_umword_t *label)
{
  return l4_ipc_gate_get_infos_u(gate, label, l4_utcb());
}
