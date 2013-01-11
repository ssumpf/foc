/**
 * \file
 * \brief ARM virtualization interface.
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
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

#include <l4/sys/ipc.h>
#include <l4/sys/task.h>

/**
 * \defgroup l4_vm_tz_api VM API for TZ
 * \brief Virtual Machine API for ARM TrustZone
 * \ingroup l4_vm_api
 */

/**
 * \brief state structure for TrustZone VMs
 * \ingroup l4_vm_tz_api
 */
struct l4_vm_state
{
  l4_umword_t r[13]; // r0 - r12

  l4_umword_t sp_usr;
  l4_umword_t lr_usr;

  l4_umword_t sp_irq;
  l4_umword_t lr_irq;
  l4_umword_t spsr_irq;

  l4_umword_t r_fiq[5]; // r8 - r12
  l4_umword_t sp_fiq;
  l4_umword_t lr_fiq;
  l4_umword_t spsr_fiq;

  l4_umword_t sp_abt;
  l4_umword_t lr_abt;
  l4_umword_t spsr_abt;

  l4_umword_t sp_und;
  l4_umword_t lr_und;
  l4_umword_t spsr_und;

  l4_umword_t sp_svc;
  l4_umword_t lr_svc;
  l4_umword_t spsr_svc;

  l4_umword_t pc;
  l4_umword_t cpsr;

  l4_umword_t pending_events;

  l4_umword_t cp15_ttbr0;
  l4_umword_t cp15_ttbr1;
  l4_umword_t cp15_ttbc;
  l4_umword_t cp15_vector_base;
  l4_umword_t cp15_dfsr;
  l4_umword_t cp15_dfar;
  l4_umword_t cp15_ifsr;
  l4_umword_t cp15_ifar;
  l4_umword_t cp15_control;
  l4_umword_t cp15_prim_region_remap;
  l4_umword_t cp15_norm_region_remap;
  l4_umword_t cp15_cid;    // banked
  l4_umword_t cp15_tls[3]; // banked
  l4_umword_t cp10_fpexc;

  l4_umword_t pfs;
  l4_umword_t pfa;
  l4_umword_t exit_reason;
};


/**
 * \brief Run a VM
 * \ingroup l4_vm_tz_api
 *
 * \param vm         Capability selector for VM
 */
L4_INLINE l4_msgtag_t
l4_vm_run(l4_cap_idx_t vm, l4_fpage_t const vm_state_fpage,
          l4_umword_t *label) L4_NOTHROW;

/**
 * \internal
 * \ingroup l4_vm_tz_api
 */
L4_INLINE l4_msgtag_t
l4_vm_run_u(l4_cap_idx_t vm, l4_fpage_t const vm_state_fpage,
            l4_umword_t *label, l4_utcb_t *u) L4_NOTHROW;

/**
 * \internal
 * \brief Operations on task objects.
 * \ingroup l4_vm_tz_api
 */
enum
{
  L4_VM_RUN_OP    = 0    /* Run a VM */
};


/****** Implementations ****************/

L4_INLINE l4_msgtag_t
l4_vm_run_u(l4_cap_idx_t vm, l4_fpage_t const vm_state_fpage,
            l4_umword_t *label, l4_utcb_t *u) L4_NOTHROW
{
  l4_msg_regs_t *r = l4_utcb_mr_u(u);
  r->mr[0] = L4_VM_RUN_OP;
  r->mr[1] = vm_state_fpage.raw;

  return l4_ipc(vm, u, L4_SYSF_CALL, *label, l4_msgtag(L4_PROTO_TASK, 3, 0, 0), label, L4_IPC_NEVER);
}

L4_INLINE l4_msgtag_t
l4_vm_run(l4_cap_idx_t vm, l4_fpage_t const vm_state_fpage,
          l4_umword_t *label) L4_NOTHROW
{
  return l4_vm_run_u(vm, vm_state_fpage, label, l4_utcb());
}
