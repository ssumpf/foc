/* -*- asm -*- */

#include "globalconfig.h"
#include "config_tcbsize.h"
#include "tcboffset.h"



/**********************************************************************
 * calculate the TCB address from a stack pointer
 */
.macro CONTEXT_OF reg, ptr
	bic	\reg, \ptr, #((THREAD_BLOCK_SIZE-1) & 0xff)
	bic	\reg, \reg, #((THREAD_BLOCK_SIZE-1) & 0xff00)
.endm

/**********************************************************************
 * Reset the thread cancel flag. 
 * Register r0 is scratched and contains the thread state afterwards
 */
.macro RESET_THREAD_CANCEL_AT tcb
	ldr 	r0, [\tcb, #(OFS__THREAD__STATE)]
	bic	r0, r0, #0x100
	str	r0, [\tcb, #(OFS__THREAD__STATE)]
.endm

/****************************
 * some handy definitions
 */
#define RF_SIZE      20
#define RF_PSR       16
#define RF_PC        12
#define RF_SVC_LR     8
#define RF_USR_LR     4
#define RF_USR_SP     0
#define RF(reg, offs) (RF_##reg + (offs))

/**************************************************************************
 * Enter kernel mode (i.e. switch from any exception mode to the 
 * kernel mode and transfer the exception state).
 */

.macro atomic_fixup insn do_store_pc
#ifndef CONFIG_ARM_V6PLUS
	@ Adjust PC if it is in the special atomic insns area
        @ Zero-flag set after this fixup code
        cmp     \insn, #0xffffe000
	bls	1f
	cmp	\insn, #0xfffff000
	bhs     1f
	tst	\insn, #0x10
	biceq	\insn, \insn, #0x0ff
.if \do_store_pc
	str	\insn, [sp, #RF(PC,  RF_SIZE)]
.endif
1:
	@ ---------------------------------------------------
#endif
.endm

.macro  __switch_to_kernel reg adjust atomic_fixup not_svc
.if \adjust != 0
	sub	lr, lr, #\adjust
.endif
#ifdef CONFIG_ARM_V6PLUS
#ifdef CONFIG_ARM_1136
	// todo: do clrex with strex for CPUs without clrex
#else
	clrex
#endif
#endif
.if \atomic_fixup
        atomic_fixup lr 0
.endif
#if defined(CONFIG_ARM_V6PLUS)
	srsdb   #0x13!
	msr	cpsr_c, #0xd3 @cpsid   f, #0x13
	str	lr, [sp, #-4]!
#else
.if \not_svc
	str	lr, s_lr
	mrs	\reg, spsr
	str	\reg, s_spsr
	msr	cpsr_c, #0xd3
.endif  @ syscall (already in svc mode)
	str	lr, [sp, #-12]!
.if \not_svc
	ldr	lr, s_spsr
	str	lr, [sp, #RF(PSR, -8)]
	ldr	lr, s_lr
	str	lr, [sp, #RF(PC,  -8)]
.else
	str	lr, [sp, #RF(PC, -8)]
	mrs	lr, spsr
	str	lr, [sp, #RF(PSR, -8)]
.endif
#endif
.endm
.macro  switch_to_kernel adjust atomic_fixup not_svc
	__switch_to_kernel r14 \adjust \atomic_fixup \not_svc
.endm

/*************************************************************************
 * return from an exception
 */
.macro 	return_from_exception
	ldr	lr, [sp, #RF(PSR,0)]		@ Unstack SPSR
	tst	lr, #0x0f			@ Mask all but relevant mode bits
	add	sp, sp, #RF_SIZE		@ SP to top of stack
#if defined(CONFIG_ARM_V6PLUS)
	ldrne   lr, [sp, #RF(SVC_LR, -RF_SIZE)]	@ load old kernel lr
	rfedb	sp
#else
	msr	spsr_cfsx, lr			@ Load SPSR from kernel_lr
	ldr	lr, [sp, #RF(PC, -RF_SIZE)]	@ copy PC on psr field for
	str	lr, [sp, #RF(PSR, -RF_SIZE)]	@   final ldmdb and proper ksp
        ldrne   lr, [sp, #RF(SVC_LR, -RF_SIZE)]	@ load old kernel lr
	ldmdb	sp, {pc}^			@ go back to interrupted insn 
						@ (load psr)
#endif
.endm


/***********************************************************************
 * Enter the kernel slowtrap handler
 *
 * Stack the state and call 'slowtrap_entry' with sp and error code
 */
.macro 	enter_slowtrap_w_stack errorcode
	mov	r1, #\errorcode
	stmdb	sp!, {r0, r1}
	mov 	r0, sp
	adr	lr, exception_return
	ldr	pc, .LCslowtrap_entry
.endm

.macro 	enter_slowtrap errorcode
	stmdb 	sp!, {r0 - r12}
	enter_slowtrap_w_stack \errorcode
.endm

/*	
 * Stack layout:
 *
 *  after SWITCH_TO_SVC !!!!
 *
 *             |       |
 *	       +-------+
 *             |  lr'  |  (pc after syscall)
 *             +-------+
 *   old sp -> | spsr  |
 *	       +-------+
 *             | km_lr |
 *             +-------+
 *             |  lr^  |  (user lr)
 *             +-------+
 *       sp -> |  sp^  |  (user sp)
 *             +-------+
 *             |       |
 *
 */


/*************************************************************************
 *
 * Generate stack for exception entries
 * - Adjust return address
 * - Store return address at [sp + 8]
 * - Store spsr at [sp + 4]
 * - sp := sp - 8
 * - Store user sp at [sp]
 * - Store user lr at [sp + 4]
 */
.macro	exceptionframe
	sub	sp, sp, #8
@	stmia	sp, {sp,lr}^ @ now done lazy
.endm


/***************************************************************************
 * Generate stack for system call entries
 *
 * Stack layout:
 *
 *  after SWITCH_TO_SVC !!!!
 *
 *             |       |
 *	       +-------+
 *             |  lr^  |  (pc after syscall)
 *             +-------+
 *   old sp -> | spsr  |
 *	       +-------+
 *             | km_lr |
 *             +-------+
 *             |  xx   |
 *             +-------+
 *       sp -> |  sp^  |  (user sp)
 *             +-------+
 *             |       |
 *
 *
 * lr: must contain fault addr (from switch_to_kernel)
 */
.macro	syscallframe	
	add	lr, sp, #RF(PC, -8)
	stmia	lr, {lr}^
	sub	sp, sp, #8
@	stmia	sp, {sp}^ @ now done lazy
.endm

.macro  enter_sys_call no_sys_call
	ldr	lr, [sp, #RF(PC,  -8)]
	cmn	lr, #0x2a		@ Range Check !!! UNSIGNED !!!
	bls	\no_sys_call		@ no syscall
	cmn	lr, #0x08
	bhi	\no_sys_call
	syscallframe			
	stmdb   sp!, {r0 - r12}
	CONTEXT_OF	r1, sp
	RESET_THREAD_CANCEL_AT r1       @ sets r0 to state 
	tst	r0, #0x810000
	bne	alien_syscall
	ldr	r0, [sp, #RF(SVC_LR, 13*4)]     @ read exception PC from stack (km_lr)
	adr	r1, sys_call_table
	sub	r0, r1, r0
	adr	lr, 2f
1:	ldr	pc, [r0]
.global fast_ret_from_irq
fast_ret_from_irq:
2:      ldmia   sp, {r0 - r12}^
        msr	cpsr_c, #0xd3 // disable IRQs
        add     sp, sp, #13*4

	/* Return */
	ldr	lr, [sp, #RF(PSR,0)]
	msr	spsr_cfsx, lr
@	ldmia	sp, {sp,lr}^ @ done lazy
	add	sp, sp, #RF_SIZE
	ldr	lr, [sp, #RF(PC, -RF_SIZE)]
	movs	pc, lr
.endm


/**************************************************************************
 * The Exception vector table.
 */
.section	.vect,"a"
.globl	exception_vector
exception_vector:
	nop				/* RESET	*/
	b	undef_entry		/* UNDEF	*/
	b	swi_entry		/* SWI		*/
	b	inst_abort_entry	/* IABORT	*/
	b	data_abort_entry	/* DABORT	*/
	nop				/* reserved	*/
	b	irq_entry		/* IRQ		*/
	b	fiq_entry		/* FIQ		*/


/* locations to pass lr and spsr from one mode to the other
   these are globally shared !!! */
.section	.excp.text,"xa"
#if !defined(CONFIG_ARM_V6PLUS)
s_lr:	.word	0
s_spsr:	.word	0
#endif

/***************************************************************************
**
** Exception entry points.
**
*/

/***************************************************************************
 * Exception undef ()
 *
 *    Exception is an undefined instruction.
 *
 */
undef_entry:
	switch_to_kernel 0 0 1
	exceptionframe
	enter_slowtrap 0x00100000

/**************************************************************************
 * Exception swi ()
 *
 *    Exception is a software interrupt (typically a syscall in normal
 *    OSes).
 *
 */
swi_entry:
	switch_to_kernel 0 0 0
	enter_sys_call no_sys_call
no_sys_call:
	exceptionframe
	enter_slowtrap 0x00200000

.align 4

/***************************************************************************
 * Exception inst_abort ()
 *
 *    Exception is a prefetch (instruction) abort.  This exception is also
 *    used for L4 syscalls.  If the exception address is in the range 0x00
 *    to 0x24 (in the exception vector page), this is interpreted as a
 *    syscall number.  Some platforms allow the exception vector to be
 *    relocated to the beginning of the last 64K of memory.  For these
 *    platforms, we use a negative (i.e. end of address space) value to
 *    indicate the syscall number.  If exception is not within the syscall
 *    range, generate a pager IPC (or panic if within the kernel).
 *
 */
inst_abort_entry:

	switch_to_kernel 4 0 1

	
/**************************************************************************/
prefetch_abort: @ A real prefetch abort occured --- handled as a page fault 
	exceptionframe
	stmdb   sp!, {r0 - r3, r12} 	@ Stack rest of user state
	ldr	lr, [sp, #RF(PSR, 5*4)]	@ get spsr from stack
	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
	bne	kernel_prefetch_abort	@ Kernel abort?
	/* user prefetch abort */
	mrc     p15, 0, r1, c5, c0, 1	@ Load IFSR into r1
	bic	r1, r1, #0x00ff0000
	orr	r1, r1, #0x00330000     @ Set read bit and prefetch abort
#if defined(CONFIG_ARM_V6PLUS) && !defined(CONFIG_ARM_1136) && !defined(CONFIG_ARM_MPCORE)
	mrc	p15, 0, r0, c6, c0, 2   @ Read fault address, for T2: pfa != pc
#else
	ldr	r0, [sp, #RF(PC, 5*4)]  @ Get PC from RF and use as pfa
#endif
	mov	r2, r0
	add 	r3, sp, #(5*4)
	stmdb	sp!, {r0, r1}
	adr	lr, pagefault_return
	ldr	pc,  .LCpagefault_entry	@ Jump to C code

kernel_prefetch_abort:			@ Kernel generated IAbort
					@ Should not get IAborts in kernel
					@ Kernel Panic
	adr	r0, kernel_prefetch_abort_label
	b	kern_kdebug_entry





/****************************************************************************
 * Exception data_abort ()
 *
 *    Exception is a data abort.  If exception happened in user mode,
 *    generate pager IPC.  If exception happened in kernel mode, it is
 *    probably due to a non-mapped TCB (or else we panic).
 *
 *
 * Stack layout:
 *
 *   old sp->  |       |
 *             +-------+
 *             |  lr'  | +68
 *             +-------+
 *             | spsr  | +64
 *	       +-------+
 *             | km_lr | +60
 *             +-------+
 *             |  ulr  | +56
 *             +-------+
 *             |  usp  | +52
 *             +-------+
 *             |  r12  | +48
 *             +-------+
 *               :  : 	
 *             +-------+
 *       sp -> |  r0   | +0
 *             +-------+
 *             |       |
 *
 *
 *
 */

.macro check_ldrd_insn jmp_to_if_ldrd
	tst	r3, #0x0e000000
	bne	1f
	and	r12, r3, #0x000000f0
	cmp	r12, #0x000000d0
	bne	1f
	tst	r3, #(1<<20)
	beq	\jmp_to_if_ldrd
1:
.endm

data_abort_entry:
	switch_to_kernel 8 0 1

	exceptionframe
	stmdb   sp!, {r0 - r3, r12}          @ Stack rest of user state

	/* user data abort */
#ifdef CONFIG_ARM_V6PLUS
	mrc     p15, 0, r1, c5, c0, 0	@ Load DFSR into r1
	bic	r1, r1, #0x00ff0000
	mrc     p15, 0, r0, c6, c0, 0	@ Load DFAR into r0

	ldr	r2, [sp, #RF(PC,  5*4)] @ Load PC into r2
	ldr	lr, [sp, #RF(PSR, 5*4)]	@ load spsr, from stack 

	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
					@ NE -> kernel
	add 	r3, sp, #(5*4)
	orreq	r1, r1, #0x00010000
	orr	r1, r1, #0x00400000     @ Set error code to data abort

	stmdb	sp!, {r0, r1}
	adr	lr, pagefault_return	@ set return address

	ldr	pc, .LCpagefault_entry	@ page fault	
#else
	mrc     p15, 0, r1, c5, c0, 0	@ Load FSR into r1
	bic	r1, r1, #(1 << 11)	@ clear bit 11 (write indicator)
	bic	r1, r1, #0x00ff0000
	mrc     p15, 0, r0, c6, c0, 0	@ Load FAR into r0
	ldr	r2, [sp, #RF(PC,  5*4)] @ Load PC into r2
	ldr	lr, [sp, #RF(PSR, 5*4)]	@ load spsr, from stack 
	tst	lr, #0x20		@ comes from thumb mode?
	bne	.LChandle_thumb
	@ arm insns
	ldr	r3, [r2]		@ Load faulting insn
	check_ldrd_insn .LCwas_ldrd
	tst	r3, #(1<<20)
	orreq	r1, r1, #(1 << 11)	@ Set FSR write bit
.LCret_handle_thumb:
.LCwas_ldrd:
	atomic_fixup r2 1
	ands	lr, lr, #0x0f		@ Mask all but relevant mode bits
					@ NE -> kernel
	add 	r3, sp, #(5*4)
	orreq	r1, r1, #0x00010000
	orr	r1, r1, #0x00400000     @ Set error code to data abort
	stmdb	sp!, {r0, r1}
	adr	lr, pagefault_return	@ set return address

	ldr	pc, .LCpagefault_entry	@ page fault

.LChandle_thumb:
	@ thumb insns
	ldrh	r3, [r2]
	and     r3, r3, #0xfe00
	teq     r3, #0x5600
	beq     .LCret_handle_thumb
	tst     r3, #(1<<11)
	orreq   r1, r1, #(1 << 11)	@ Set FSR write bit
	b .LCret_handle_thumb
#endif

.LCpagefault_entry:	.word	pagefault_entry
.LCslowtrap_entry:	.word	slowtrap_entry


/***************************************************************************
 * Generic return code for restoring the thread state after exceptions.
 *
 * Stack layout:
 *
 *	 sp->  |       |
 *             +-------+
 *             |  lr'  | +68
 *             +-------+
 *             | spsr  | +64
 *             +-------+
 *             | km_lr | +60
 *             +-------+
 *             |  ulr  | +56
 *             +-------+
 *             |  usp  | +52
 *             +-------+
 *             |  r12  | +48
 *             +-------+
 *               :  : 	
 *             +-------+
 *   old sp -> |  r0   | +0
 *             +-------+
 *             |       |
 *
 *
 */	
pagefault_return:
	cmp 	r0, #0
	ldmia	sp!, {r12, lr}
	beq	slowtrap_from_pagefault
	
	msrne	cpsr_c, #0xd3 // disable IRQs
	ldmneia	sp!, {r0 - r3, r12}		@ Restore user state
	return_from_exception

slowtrap_from_pagefault:
        msr	cpsr_c, #0xd3 // disable IRQs
	ldmia 	sp!, {r0 - r3}
	stmdb 	sp!, {r0 - r11}
	stmdb 	sp!, {r12, lr}
	mov	r0, sp
	adr   	lr, exception_return
	ldr	pc, .LCslowtrap_entry	@ slow trap

	.global __return_from_exception
__return_from_exception:
exception_return:
	msr	cpsr_c, #0xd3 // disable IRQs
	add	sp, sp, #8
	ldmia	sp!, {r0 - r12}
	return_from_exception

	.align 4
	.global __iret
__iret:
	return_from_exception


/***************************************************************************
 * Exception irq ()
 *
 *    Exception is an interrupt.  Generate interrupt IPC.
 *
 */
irq_entry:
	switch_to_kernel 4 1 1
	exceptionframe

	stmdb   sp!, {r0 - r3, r12}   	@ Stack rest of user state
	@ add r0, sp, #(5*4) @ debug
        mov     lr, pc
        ldr     pc, 1f
	ldmia	sp, {r0 - r3, r12}		@ Restore user state
        msr	cpsr_c, #0xd3 // disable IRQs
	add	sp, sp, #20
	return_from_exception
1:	.word	irq_handler


/******************************************************************************
 * Exception fiq ()
 *
 *    Exception is a fast interrupt.
 *
 */
fiq_entry:
	switch_to_kernel 4 1 1
	exceptionframe

	stmdb   sp!, {r0 - r3, r12}   	@ Stack rest of user state
	@ add r0, sp, #(5*4) @ debug
	mov     lr, pc
	ldr     pc, 1f
	ldmia	sp, {r0 - r3, r12}		@ Restore user state
	msr	cpsr_c, #0xd3 // disable IRQs
	add	sp, sp, #20
	return_from_exception
1:	.word	irq_handler


/**************************************************************************/
/* The alien stuff is below                                               */
/**************************************************************************/
alien_syscall: @ Do it for an alien ---------------------------------------
	tst	r0, #0x20000
	bicne	r0, r0, #0x20000
	bne	1f
	@ Trap alien before system call -----------------------------------
	@ The trap is an insn abort on the syscall address in the kernel.
	ldr	lr, [sp, #RF(PC,     13*4)]
	str	lr, [sp, #RF(USR_LR, 13*4)]
	ldr	lr, [sp, #RF(SVC_LR, 13*4)] @ read orig exception PC
	sub	lr, lr, #4                  @ adjust pc to be on insn
	str	lr, [sp, #RF(PC,     13*4)] @ store to entry_stack_PC
	enter_slowtrap_w_stack 0x00300000
	@ Never reach this -- end up in user land after exception reply

1:	@ Resume the alien system call ------------------------------------
	str	r0, [r1, #(OFS__THREAD__STATE)]
	ldr	r0, [sp, #RF(SVC_LR, 13*4)] @ read orig excpetion PC
	adr	r1, sys_call_table
	sub	r0, r1, r0
	adr	lr, 2f
	ldr	pc, [r0]
2:	nop @ The return point after the resumed alien system call --------
	msr	cpsr_c, #0xd3 // disable IRQs
	@ Trap after the resumed alien system call ------------------------
	@ The trap occurs at the insn where the system call returns to.
	@ Set the bit 0x00010000 to indicate a trap after the resumed 
	@ system call.
	enter_slowtrap_w_stack 0x00310000

	
/*****************************************************************************/
/* The syscall table stuff                                                   */
/*****************************************************************************/
#define SYSCALL(name) .word sys_##name##_wrapper

.globl sys_call_table
sys_call_table:
	.word sys_kdb_ke
	.word sys_kdb_ke
/*SYSCALL(ipc)*/
	.word sys_ipc_wrapper
	.word sys_arm_mem_op
SYSCALL(invoke_debug)
	.word sys_kdb_ke
	.word sys_kdb_ke
	.word sys_kdb_ke
	.word sys_kdb_ke
	.word sys_kdb_ke
	.word sys_kdb_ke

	.align 4	
	.global leave_by_trigger_exception

leave_by_trigger_exception:
	sub 	sp, sp, #RF_SIZE   @ restore old return frame
	stmdb 	sp!, {r0 - r12}

	/* restore original IP */
	CONTEXT_OF r1, sp
	ldr 	r0, [r1, #(OFS__THREAD__EXCEPTION_IP)]
	str	r0, [sp, #RF(PC, 13*4)]

	ldr	r0, [r1, #(OFS__THREAD__EXCEPTION_PSR)]
	str	r0, [sp, #RF(PSR, 13*4)]

	mov     r0, #~0
	str	r0, [r1, #(OFS__THREAD__EXCEPTION_IP)]

	enter_slowtrap_w_stack 0x00500000

	.align 4	
	.global leave_by_vcpu_upcall;

leave_by_vcpu_upcall:
	sub 	sp, sp, #RF_SIZE   @ restore old return frame
	stmdb 	sp!, {r0 - r2}

	/* restore original IP */
	CONTEXT_OF r1, sp

	/* access_vcpu() for the local case */
	ldr	r2, [r1, #(OFS__THREAD__USER_VCPU)]
	add	r2, r2, #(VAL__SIZEOF_TRAP_STATE - RF_SIZE)

	ldr 	r0, [r1, #(OFS__THREAD__EXCEPTION_IP)]
	str	r0, [r2, #RF(PC, 0)]

	ldr	r0, [r1, #(OFS__THREAD__EXCEPTION_PSR)]
	str	r0, [r2, #RF(PSR, 0)]
	bic	r0, #0x20 // force ARM mode
	str	r0, [sp, #RF(PSR, 3*4)]

	mov     r0, #~0
	str	r0, [r1, #(OFS__THREAD__EXCEPTION_IP)]

	ldr	r0, [sp, #RF(USR_LR, 3*4)]
	str	r0, [r2, #RF(USR_LR, 0)]

	ldr	r0, [sp, #RF(USR_SP, 3*4)]
	str	r0, [r2, #RF(USR_SP, 0)]

	stmdb   r2!, {r3-r12}

	ldr	r0, [sp, #8]
	str	r0, [r2, #-4]

	ldr	r0, [sp, #4]
	str	r0, [r2, #-8]

	ldr	r0, [sp]
	str	r0, [r2, #-12]!

	add	sp, sp, #(3*4)

	add	r0, r2, #(-8 + OFS__VCPU_STATE__ENTRY_SP)
	ldm	r0, {sp}^

	ldr	r0, [r2, #(-8 + OFS__VCPU_STATE__ENTRY_IP)]

	str	r0, [sp, #RF(PC, 0)]
	add	r0, r2, #(-8)

	b	__iret


kernel_prefetch_abort_label: .string "Kernel prefetch abort"
missed_excpt_ret_label:      .string "ERROR in exception return"
fiq_label:	             .string "FIQ entry"

/**********************************************************************
	kdebug entry
 **********************************************************************/

.macro DEBUGGER_ENTRY type
#ifdef CONFIG_JDB
	str	sp, [sp, #(RF(USR_SP, -RF_SIZE))] @ save r[13]
	sub	sp, sp, #(RF_SIZE)

	str	lr, [sp, #RF(SVC_LR, 0)]
	str	lr, [sp, #RF(PC, 0)]
        mrs	lr, cpsr
	str	lr, [sp, #RF(PSR, 0)]

	stmdb	sp!, {r0 - r12}
	mov	r0, #-1			@ pfa
	mov	r1, #0x00e00000		@ err
	orr	r1, #\type		@ + type
	stmdb	sp!, {r0, r1}

	mov	r0, sp
	adr	lr, 1f
	ldr	pc, 3f

1:
	add	sp, sp, #8		@ pfa and err
	ldmia	sp!, {r0 - r12}
	ldr	lr, [sp, #RF(PSR, 0)]
	msr	cpsr, lr
	ldr	lr, [sp, #RF(SVC_LR, 0)]

	ldr	sp, [sp, #(RF(USR_SP, 0))]
	mov	pc, lr


3:	.word call_nested_trap_handler
#else
	mov	pc, lr
#endif
.endm

	.global	kern_kdebug_entry
	.align 4
kern_kdebug_entry:
	DEBUGGER_ENTRY 0

	.global	kern_kdebug_sequence_entry
	.align 4
kern_kdebug_sequence_entry:
	DEBUGGER_ENTRY 1


#ifdef CONFIG_MP
	.section ".text"
	.global	kern_kdebug_ipi_entry
	.align 4
kern_kdebug_ipi_entry:
	DEBUGGER_ENTRY 2
	.previous
#endif



#ifdef CONFIG_ARM_TZ

.macro ISB_OP reg
#ifdef CONFIG_ARM_V7
	isb
#else
	mcr p15, 0, lr, c7, c5, 4	@ cp15isb
#endif
.endm

/**********************************************************************
 * Secure and Nonsecure switching stuff
 *
 *********************************************************************/
.macro SAVE_NONSECURE_STATE off

	// save exit reason temporarily on stack
	str	lr, [sp,#-12]
	
	// switch to secure world
	mov	lr, #0
	mcr	p15, 0, lr, c1, c1, 0
	
	// save gen-regs
	ldr	lr, [sp, #\off]
	//add	lr, lr, #8
	stmia	lr!, {r0 - r12}

	mov	r0, lr

	// usr
	stmia	r0, {sp, lr}^
	add	r0, r0, #8

	// irq
	cps	#0x12
	stmia	r0!, {sp, lr}
	mrs	r1, spsr
	stmia	r0!, {r1}
	
	// fiq
	cps	#0x11
	stmia	r0!, {r8 - r12, sp, lr}
	mrs	r1, spsr
	stmia	r0!, {r1}

	// abt
	cps	#0x17
	stmia	r0!, {sp, lr}
	mrs	r1, spsr
	stmia	r0!, {r1}

	// und
	cps	#0x1b
	stmia	r0!, {sp, lr}
	mrs	r1, spsr
	stmia	r0!, {r1}
	
	// svc
	cps	#0x13
	stmia	r0!, {sp, lr}
	mrs	r1, spsr
	stmia	r0!, {r1}
	
	cps	#0x16
	
	// copy return pc/cpsr from stack
	sub	lr, sp, #8
	ldmia	lr, {r1, r2}
	stmia	r0!, {r1, r2}
	
	// save pending virtual interrupt state
	mrc	p15, 0, r1, c12, c1, 1	
	stmia	r0!, {r1}

	// switch to non-secure world
	mov	r1, #1
	mcr	p15, 0, r1, c1, c1, 0
	ISB_OP	r1
	
	mrc	p15, 0, r1, c2, c0, 0	@ read CP15_TTB0
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c2, c0, 1	@ read CP15_TTB1
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c2, c0, 2	@ read CP15_TTBC
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c12, c0, 0	@ read CP15_VECTOR_BASE
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c5, c0, 0	@ read CP15_DFSR
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c6, c0, 0	@ read CP15_DFAR
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c5, c0, 1	@ read CP15_IFSR
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c6, c0, 2	@ read CP15_IFAR
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c1, c0, 0	@ read CP15_CONTROL
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c10, c2, 0	@ read CP15_PRIM_REGION_REMAP
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c10, c2, 1	@ read CP15_NORM_REGION_REMAP
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c13, c0, 1	@ read CP15_CID
	stmia	r0!, {r1}

	// tls regs are banked
	mrc	p15, 0, r1, c13, c0, 2	@ read CP15_TLS1
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c13, c0, 3	@ read CP15_TLS2
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c13, c0, 4	@ read CP15_TLS3
	stmia	r0!, {r1}

	mrc	p10, 7, r1, cr8, cr0, 0 @ fpexc
	stmia	r0!, {r1}

	// switch to secure world
	mov	r1, #0
	mcr	p15, 0, r1, c1, c1, 0
	ISB_OP	r1
	
	mrc	p15, 0, r1, c5, c0, 0	@ read CP15_DFSR
	stmia	r0!, {r1}

	mrc	p15, 0, r1, c6, c0, 0	@ read CP15_DFAR
	stmia	r0!, {r1}

	// copy the exit reason from stack
	ldr	r1, [sp, #-12]
	stmia	r0!,{r1}
.endm

.macro RESTORE_NONSECURE_STATE off
	
	ldr	r0, [sp, #\off]

	// jump over general purpose register
	add	r0, r0, #13*4

	// usr
	ldmia	r0, {sp, lr}^
	add	r0, r0, #8

	// irq
	cps	#0x12
	ldmia	r0!, {sp, lr}
	ldmia	r0!, {r1}
	msr	spsr, r1
	
	// fiq
	cps	#0x11
	ldmia	r0!, {r8 - r12, sp, lr}
	ldmia	r0!, {r1}
	msr	spsr, r1

	// abt
	cps	#0x17
	ldmia	r0!, {sp, lr}
	ldmia	r0!, {r1}
	mrs	r1, spsr

	// und
	cps	#0x1b
	ldmia	r0!, {sp, lr}
	ldmia	r0!, {r1}
	msr	spsr, r1
	
	// svc
	cps	#0x13
	ldmia	r0!, {sp, lr}
	ldmia	r0!, {r1}
	msr	spsr, r1

	cps	#0x16

	// copy return pc/cpsr on stack
	ldmia	r0!, {r1, r2}
	stmdb	sp, {r1, r2}

	// set pending events
	ldmia	r0!, {r1}
	and	r1, r1, #0x1c0
	mcr	p15, 0, r1, c12, c1, 1

#if 1
	// switch to non-secure world
	mov	r1, #1
	mcr	p15, 0, r1, c1, c1, 0
	ISB_OP	r1

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c2, c0, 0	@ write CP15_TTB0

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c2, c0, 1	@ write CP15_TTB1

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c2, c0, 2	@ write CP15_TTBC

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c12, c0, 0	@ write CP15_VECTOR_BASE

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c5, c0, 0	@ write CP15_DFSR

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c6, c0, 0	@ write CP15_DFAR

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c5, c0, 1	@ write CP15_IFSR

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c6, c0, 2	@ write CP15_IFAR

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c1, c0, 0	@ write CP15_CONTROL

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c10, c2, 0	@ write CP15_PRIM_REGION_REMAP

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c10, c2, 1	@ write CP15_NORM_REGION_REMAP

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c13, c0, 1	@ write CP15_CID

	// tls regs are banked
	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c13, c0, 2	@ write CP15_TLS1

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c13, c0, 3	@ write CP15_TLS2

	ldmia	r0!, {r1}
	mcr	p15, 0, r1, c13, c0, 4	@ write CP15_TLS3

	ldmia	r0!, {r1}
        mcr p10, 7, r1, cr8, cr0, 0     @ fpexc

	// switch to secure world
	mov	r1, #0
	mcr	p15, 0, r1, c1, c1, 0
	ISB_OP	r1
#endif

	// load gen-regs
	ldr	lr, [sp, #\off]
	ldmia	lr!, {r0 - r12}
.endm

/**********************************************************************
 * Save secure state on top of the stack.
 *
 * We save also the user-level registers here, because we need to
 * restore some on FIQ.
 *
 */
.macro SAVE_SECURE_STATE

	stmdb	sp!, {r3, r4}	@ save supervisor return values
	stmdb	sp, {sp, lr}^	@ save user-level return values
	sub	sp, sp, #8
.endm

/**********************************************************************
 * Restore secure state when guest returns with monitor call.
 *
 * This removes the secure state from the top of the stack.
 */
.macro RESTORE_SECURE_STATE

	mov	r0, sp		@ restore stack pointer from supervisor mode
	cps	#0x13
	mov	sp, r0
	cps	#0x16
	ldmia	sp, {sp, lr}^	@ restore user-level return values
	add	sp, sp, #8
	ldmia	sp!, {r3, r4}	@ restore supervisor return values
.endm

/**********************************************************************
 * Restore secure state when guest is interrupted by FIQ
 *
 * Don't remove secure state from stack as we need it
 * when application guest exits.
 * Just restore user-level state as this is spilled by the irq handler
 */
.macro RESTORE_SECURE_STATE_FIQ

	mov	r0, sp		@ restore stack pointer from supervisor mode
	cps	#0x13
	mov	sp, r0
	cps	#0x16
	ldmia	sp, {sp, lr}^	@ restore user-level return values
.endm

.macro SWITCH_TO_NONSECURE_MODE
	mov	lr, #0xf
	mcr	p15, 0, lr, c1, c1, 0
	ISB_OP	lr
.endm

.macro SWITCH_TO_SECURE_MODE
	mov	lr, #0x0
	mcr	p15, 0, lr, c1, c1, 0
	ISB_OP	lr
.endm


/*****************************************************************************/
/* The monitor entry table stuff                                             */
/*****************************************************************************/
.p2align 5
.globl monitor_vector_base
monitor_vector_base:
	nop				/* RESET	*/
	b	mon_undef_entry		/* UNDEF	*/
	b	mon_swi_entry		/* SWI		*/
	b	mon_inst_abort_entry	/* IABORT	*/
	b	mon_data_abort_entry	/* DABORT	*/
	nop				/* reserved	*/
	b	mon_irq_entry		/* IRQ		*/
	b	mon_fiq_entry		/* FIQ		*/


mon_undef_entry:
1:	b 1b

mon_swi_entry:
	srsdb	sp, #0x16		@ save return state temporarily on stack
	mov	lr, #1			@ set exit reason
	b	go_secure
	
mon_inst_abort_entry:
	sub	lr, lr, #4
	srsdb	sp, #0x16
	mov	lr, #2			@ set exit reason
	b	go_secure
	
mon_data_abort_entry:
	sub	lr, lr, #4
	srsdb	sp, #0x16
	mov	lr, #3			@ set exit reason
	b	go_secure
	
mon_irq_entry:
	sub	lr, lr, #4
	srsdb	sp, #0x16
	mov	lr, #4			@ set exit reason
	b	go_secure

mon_fiq_entry:
	sub	lr, lr, #4		@ adjust saved ip
	srsdb	sp, #0x16
	mov	lr, #4			@ set exit reason
	b	go_secure

//	cps	#0x12			@ switch to irq mode
//	adr	lr, go_nonsecure_after_fiq + 4			@ set lr_irq
//	msr	spsr, #0xd3		@ set spsr_irq
//	b	fiq_entry

/**********************************************************************
 * Go to secure world
 *
 */
go_secure:
	SAVE_NONSECURE_STATE 16
	RESTORE_SECURE_STATE
	
	mov	lr, r3
	msr	spsr_cfsx, r4
	movs	pc, lr
	
/**********************************************************************
 * Go to nonsecure world
 *
 * When the guest was interrupted by an FIQ, we don't need to save
 * secure state again, because it is still on top of the stack.
 *
 */
//go_nonsecure_after_fiq:
//	mov	r2, sp			@ copy sp_svc to sv_mon
//	cps	#0x16
//	mov	sp, r2
//	cps	#0x13
//	b go_nonsecure_after_fiq_2

.globl go_nonsecure
go_nonsecure:
	SAVE_SECURE_STATE
	RESTORE_NONSECURE_STATE 16
	SWITCH_TO_NONSECURE_MODE

//	mcr p15, 0, lr, c7, c10, 4	@ drain write buffer
//	mcr p15, 0, lr, c8, c7, 0	@ flush TLB entry

	ldr	lr, [sp, #-4]
	msr	spsr, lr		@ set spsr_mon with unsecure spsr
	ldr	lr, [sp, #-8]		@ set lr_mon with unsecure ip
	movs	pc, lr
#endif

/* -------------------------------------- TEXT ---------------------------*/

.text
	.global	vcpu_resume
vcpu_resume:
	add	sp, r1, #RF_SIZE
	add	lr, r0, #8
	ldr	r1, [lr, #RF(PSR, 13*4)]	@ Unstack SPSR
	msr	spsr, r1			@ Load SPSR from kernel_lr
	ldmia	lr!, {r0 - r12}
	ldmia	lr, {sp,lr}^			@ restore user sp and lr (now lazy)
#if defined(CONFIG_ARM_V6PLUS)
	add	lr, lr, #RF_SIZE		@ Read return address
	rfedb	lr
#else
	add	lr, lr, #(RF_SIZE - 4)		@ Read return address
	ldmdb	lr, {pc}^			@ go back to interrupted insn 
#endif
