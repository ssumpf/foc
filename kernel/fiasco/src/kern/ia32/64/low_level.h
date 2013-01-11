#ifndef LOW_LEVEL_H
#define LOW_LEVEL_H

#include "asm.h"
#include "regdefs.h"
#include "shortcut.h"
#include "tcboffset.h"


#define REGISTER_SIZE 8

	.macro save_all_regs
	push	%rax
	push	%rcx
	push	%rdx
	push	%rbx
	mov	%cr2,%rax
	push	%rax
	push	%rbp
	push	%rsi
	push	%rdi
	push	%r8
	push	%r9
	push	%r10
	push	%r11
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	.endm

	.macro restore_all_regs
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%r11
	pop	%r10
	pop	%r9
	pop	%r8
	pop	%rdi
	pop	%rsi
	pop	%rbp
	add	$0x08,%rsp		/* pop cr2 */
	pop	%rbx
	pop	%rdx
	pop	%rcx
	pop	%rax
	.endm

	.macro	SAVE_STATE_SYSEXIT
	push	%rax
	push	%rbp
	push	%rbx
	push	%rdi
	push	%rsi
	push	%rdx
	push	$0		/* rcx contains user rip */
	push	%r8
	push	%r9
	push	%r10
	push	$0		/* r11 contains user rflags */
	push	%r12
	push	%r13
	push	%r14
	push	$0		/* r15 contains user rsp */
	.endm

	.macro	RESTORE_STATE_SYSEXIT
	add	$8,%rsp		/* r15 contains user rsp */
	pop	%r14
	pop	%r13
	pop	%r12
	add	$8,%rsp		/* r11 contains user rflags */
	pop	%r10
	pop	%r9
	pop	%r8
	add	$8,%rsp		/* rcx contains user eip */
	pop	%rdx
	pop	%rsi
	pop	%rdi
	pop	%rbx
	pop	%rbp
	pop	%rax
	.endm


	.macro	DO_SYSEXIT
	RESTORE_STATE_SYSEXIT
	//	CHECK_SANITY $3		/* scratches ecx */
	//	RESTORE_IOPL
	mov	(%rsp), %rcx
        /* make RIP canonical, workaround for intel IA32e flaw */
        shl     $16, %rcx
        sar     $16, %rcx
	mov	16(%rsp), %r11	/* load user rflags */
	mov	24(%rsp), %rsp	/* user esp */
	/* mmh, maybe weird things will
	   happen, if we set an hardware breakpoint before the sysretq
	   maybe we should use the IST stuff to enforce always
	   an good kernel stack
	*/
	sysretq
	.endm

	.macro	RESET_THREAD_CANCEL_AT reg
	andl	$~(Thread_cancel | Thread_dis_alien), OFS__THREAD__STATE (\reg)
	.endm

	.macro	RESET_THREAD_IPC_MASK_AT reg
	andl	$~Thread_ipc_mask, OFS__THREAD__STATE (\reg)
	.endm

	.macro	ESP_TO_TCB_AT reg
	mov	%rsp, \reg
	andq	$~(THREAD_BLOCK_SIZE - 1), \reg
	.endm

	.macro	SAVE_STATE
	push	%rbp
	push	%rbx
	push	%rdi
	push	%rsi
	push	%rdx
	push	%rcx
	push	%r8
	push	%r9
	push	%r10
	push	%r11
	push	%r12
	push	%r13
	push	%r14
	push	%r15
	.endm

	.macro	RESTORE_STATE
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%r11
	pop	%r10
	pop	%r9
	pop	%r8
	pop	%rcx
	pop	%rdx
	pop	%rsi
	pop	%rdi
	pop	%rbx
	pop	%rbp
	.endm

#define SCRATCH_REGISTER_SIZE 72
	.macro	SAVE_SCRATCH
	push	%rdi
	push	%rsi
	push	%rax /* must be ax cx dx for pagein_tcb_request */
	push	%rcx
	push	%rdx
	push	%r8
	push	%r9
	push	%r10
	push	%r11
	.endm

	.macro	RESTORE_SCRATCH
	pop	%r11
	pop	%r10
	pop	%r9
	pop	%r8
	pop	%rdx
	pop	%rcx
	pop	%rax
	pop	%rsi
	pop	%rdi
	.endm

	.macro	IRET_INSN
	iretq
	.endm

#define PAGE_FAULT_ADDR	%cr2
#define PAGE_DIR_ADDR	%cr3

#endif //LOW_LEVEL_H
