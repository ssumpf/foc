
#include "config_gdt.h"
#include "config_tcbsize.h"
#include "globalconfig.h"
#include "idt_init.h"
#include <low_level.h>
#include "shortcut.h"
#include "tcboffset.h"
#include "regdefs.h"
#include "asm.h"

#define L4_IPC_RECANCELED		0x40
#define L4_IPC_RETIMEOUT		0x20

#define	PDIR_IDX(virt)			(((virt) >> 22) & 0x3ff)

/* stack layout
	SS
	ESP
	EFL
	CS
	EIP
	
	%rax	
	%rbp
	%rbx
	%rdi
	%rsi
	%rdx
	%rcx
	%r8
	%r9
	%r10
	%r11
	%r12
	%r13
	%r14
	%r15
*/

	
// XXXX: let save the user r8-r14 or use them as
// additional IPC registers (would be the best solution)
// then we can save and restore them lazily (only
// if we need will block. If we dont block,
// we can leave them in place.
		
#define OFS__THREAD__SS   (THREAD_BLOCK_SIZE - 1*8)
#define OFS__THREAD__ESP  (THREAD_BLOCK_SIZE - 2*8)
#define OFS__THREAD__EFL  (THREAD_BLOCK_SIZE - 3*8)
#define OFS__THREAD__CS   (THREAD_BLOCK_SIZE - 4*8)
#define OFS__THREAD__EIP  (THREAD_BLOCK_SIZE - 5*8)

// In the SYSENTER path all kernel memory accesses go through stack
// segment ss. This way we do not need to RESET_KERNEL_SEGMENTS in
// SMAS. The RESET_KERNEL_SEGMENTS function is executed if the shortcut
// fails or we switch to another thread which is not in shortcut.

	//
	// ready_enqueue
	//
	// Here we don't check if the context which is to be enqueued is
	// not current() and has the same priority as current(). In this
	// case, Context::ready_enqueue() enqueues current() first. We
	// don't do this here but the location this macro is called from
	// has to check this.
	//
	// precondition  : ecx = thread->sched()
	//		   rax = thread->sched()->prio() (upper 32bits 0)
	// scratches     : ecx, edx
	.macro	READY_ENQUEUE thread, label
	// if (prio > prio_highest)
	//   prio_highest = prio
	cmp	CONTEXT_PRIO_HIGHEST, %eax
	jbe	1f
	mov	%eax, CONTEXT_PRIO_HIGHEST

1:	mov    CONTEXT_PRIO_NEXT (, %rax, 8), %rdx
	// if (!prio_next[prio])
	or	%rdx, %rdx
	jnz	2f
	// prio_next[prio] = this;
	mov    \thread, CONTEXT_PRIO_NEXT (, %rax, 8)
	// ready_next = this;
	mov	\thread, OFS__THREAD__READY_NEXT (\thread)
	// ready_prev = this;
	mov	\thread, OFS__THREAD__READY_PREV (\thread)
	jmp	\label
2:	// ecx = prio_next[prio]->ready_prev
	mov    OFS__THREAD__READY_PREV (%rdx), %rcx
	// ready_next = prio_next[prio]
        mov    %rdx, OFS__THREAD__READY_NEXT (\thread)
	// ready_prev = prio_next[prio]->ready_prev
        mov    %rcx, OFS__THREAD__READY_PREV (\thread)
	// prio_next[prio]->ready_prev = this
	mov	\thread, OFS__THREAD__READY_PREV (%rdx)
	// ready_prev->ready_next = this
	mov	\thread, OFS__THREAD__READY_NEXT (%rcx)
	.endm


# define KIP_SWITCH_TIME   0xA8
# define KIP_CONSUMED_TIME 0xB8

	//
	// bookkeeping for the time a thread consumed
	//
	// precondition  : ebx = THIS
	//                 esi = DEST
	// scratches     : eax, ecx, edx, ebp
	.macro	CONSUME_TIME
	.endm


#ifdef CONFIG_ASSEMBLER_IPC_SHORTCUT

// only dummys

	.globl	entry_sys_ipc
entry_sys_ipc:
	push	%rax
	SAVE_STATE
	ESP_TO_TCB_AT %rbx
	RESET_THREAD_CANCEL_AT %rbx

#define THIS_rbx %rbx

	call	ipc_short_cut_wrapper
in_slow_ipc1:
	RESTORE_STATE
	pop	%rax
	iretq
	.globl	in_slow_ipc1

i30_ret_switch:
	.globl	i30_ret_switch


#ifndef CONFIG_PF_UX

// IPC entry point for sysenter. 

	.align	16
	.globl	entry_sys_fast_ipc
	.globl  entry_syscall
entry_sys_fast_ipc:
entry_syscall:
	mov	%rsp,%r15		/* save user rsp */
	mov	syscall_rsp0,%rsp	/* get address of kernel stack */
	pop	%rsp			/* set kernel stack */

/* we dont need the segment stuff */
//	push	$(GDT_DATA_USER | SEL_PL_U)	/* fake user ss */
	sub	$8, %rsp
	push	%r15				/* save user rsp */
	push	%r11				/* save user rflags */
	
//	push	$(GDT_CODE_USER | SEL_PL_U)	/* fake user cs */
	sub	$8, %rsp
	push	%rcx				/* save user rip */

	SAVE_STATE_SYSEXIT
	ESP_TO_TCB_AT %rbx
#define THIS_rbx %rbx
	RESET_THREAD_CANCEL_AT %rbx


	// test if long send or no send at all
	test	$~0, %rax
	jnz	se_shortcut_failed

	// test if destination is L4_INVALID_ID

	// because the default op-size is 32 bit
	// i will use the 32bit regs if possible
	// to save the 64bit op-prefix
	// operations on the 32bit registers
	// will zero out the upper 32bits.
	
	cmp	$~0, %esi
	je	se_shortcut_failed

	// test if destination is L4_NIL_ID
	test	%esi, %esi
	jz	se_shortcut_failed

	// test if destination has ``next_period'' bit set
	movabsq	$0x0020000000000000, %rcx
	test	%rcx, %rsi
	jnz	se_shortcut_failed

//	int3

// at this point we need only the lower 32bits of the dest id
// the higher bits will masked out later

// the ver0 field (the lower 10bits) of the ID spawns 1024 bytes,
// so multiply it by 4
	shl	$2, %esi
	and	$TCB_ADDRESS_MASK, %rsi

// this works, because the kernel lies in the top of the
// virt. address space, where all higher bits are 1
	or	$VAL__MEM_LAYOUT__TCBS, %rsi	// dst = dst_id.lookup
#define DEST_rsi %rsi

//	int3

#define RECV_DESC_rbp		%rbp
#define RECV_DESC_rbp_low	%ebp

	// test if have receive operation
	cmp	$~0, RECV_DESC_rbp
/*	je	se_test_tcb_mapped		// no */
	// first version will handle only send+rcv
	je	se_shortcut_failed	

	// test if short receive
	cmp	$1, RECV_DESC_rbp
	ja	se_shortcut_failed		// more than 2 dwords

	// rdi contains the timeout
	// test if simple timeout
	testl	$0x0f, %edi			// rcv_to==inf => exp = 0
	jz	1f				// rcv_to==inf => o.k.
	testl	$0xff000000, %edi
	jnz	se_shortcut_failed		// (rcv_to!=inf) && (rcv_to!=0)

1:	// test if open wait and (irq attached or sender queued)
	// ebp is 0 (receive) or 1 (open wait) here
	test	RECV_DESC_rbp_low, RECV_DESC_rbp_low
	jz	se_test_tcb_mapped		// closed wait

	mov	OFS__THREAD__SENDER_FIRST (THIS_rbx), %rax
	test	%rax, %rax
	jnz	se_shortcut_failed
	or	OFS__THREAD__IRQ (THIS_rbx), %rax
	jnz	se_shortcut_failed
	jmp	se_test_tcb_mapped

//	int3
	.align	8
se_shortcut_failed:
	// shortcut failed, execute normal ipc C++ pass
	CNT_SHORTCUT_FAILED
	call	sys_ipc_wrapper
in_slow_ipc2:
	DO_SYSEXIT


	.align	16
se_test_tcb_mapped:

	lea	OFS__THREAD__STATE (DEST_rsi), %rcx // addr of dst tcb state

	// Here we could raise a pagefault. The pagefault handler notices
	// that by looking at the pagefault address. In that case the pager
	// sets the carry flag and returns immediatly.
	andl	$~0, %ss:(%rcx)		// can raise pagefault
	jc	se_shortcut_failed_1		// tcb is not paged

	testl	$(Thread_delayed_deadline | Thread_delayed_ipc), (%rcx)
	jnz	se_shortcut_failed_1

	// we assume the thread state will fit in the first 32bit
	mov	(%rcx), %eax

	and	$(Thread_receiving | Thread_send_in_progress | \
		Thread_ipc_in_progress), %eax

	mov	OFS__THREAD__PARTNER (DEST_rsi), %rdx
	
	// dst->thread_lock()->test()
	cmp	$0, \
		OFS__THREAD__THREAD_LOCK__SWITCH_LOCK__LOCK_OWNER (DEST_rsi)
	jne	se_shortcut_failed_1		// dst is locked

	lea	CAST__Thread_TO_Sender (THIS_rbx), %rcx // (Sender*)this

	//    (ipc_state == (Thread_receiving | Thread_ipc_in_progress)
	cmpb	$(Thread_ipc_in_progress | Thread_receiving), %al
	jne	se_shortcut_failed_1

	// see Receiver::sender_ok
	mov    OFS__THREAD__SENDER_FIRST (DEST_rsi), %rax

	// if DEST_esi->partner() == 0, openwait
	test	%rdx, %rdx
	jne	1f

	// sender_queue empty?
	test	%rax, %rax
	jnz	1f
	jmp	se_sender_ok

1:	// if DEST_esi->partner() == this, wait for me
	cmp	%rcx, %rdx
	jne	se_shortcut_failed_1

	jmp	se_sender_ok
se_shortcut_failed_1:
	jmp	se_shortcut_failed

	.align	16

se_sender_ok:
	CNT_SHORTCUT_SUCCESS

	// clear, we need it later
	xor	%eax, %eax

	// wake up receiver
	andl	$~(Thread_ipc_receiving_mask | \
		   Thread_ipc_in_progress), OFS__THREAD__STATE (DEST_rsi)
	orb	$Thread_ready, OFS__THREAD__STATE (DEST_rsi)

	// %eax=0 => default: no receive part => status ok

	// prepare a receive if we have one
	cmp	$~0, RECV_DESC_rbp
	je	se_do_switch_exec		// no receive part
	// we should jump, because we have ruled out receiving before

//	int3

#define REGS_rsp	%rsp
	// set_rcv_regs (regs)
	mov	REGS_rsp, OFS__THREAD__RCV_REGS (THIS_rbx)

        orb	$(Thread_receiving | Thread_ipc_in_progress),\
		 OFS__THREAD__STATE (THIS_rbx)

        // default: open wait
	xor	%ecx, %ecx

        test    RECV_DESC_rbp_low, RECV_DESC_rbp_low    // open wait?
	jnz	1f					// openwait cmp yes

        // set dst's partner
	lea	CAST__Thread_TO_Sender (DEST_rsi), %rcx // (Sender*)dst

1:	mov	%rcx, OFS__THREAD__PARTNER (THIS_rbx)

	// timeout = 0
	movb	$L4_IPC_RETIMEOUT, %al
	testl	$0x0f, %edi			// rcv_to==inf => exp = 0
	jne	se_do_switch_exec		// timeout==inf? no

	// timeout = infinite ==> need wakeup
	movb	$L4_IPC_RECANCELED, %al
	andb	$~Thread_ready, OFS__THREAD__STATE (THIS_rbx)

	.align	16
se_do_switch_exec:

	mov	%rax, REG_RAX (REGS_rsp)	// store ipc result

	CNT_CONTEXT_SWITCH

        mov     OFS__THREAD__STATE (THIS_rbx), %eax
	test	$Thread_fpu_owner, %eax
	jz	1f
	// set ts
	mov	%cr0, %rdx
	or	$CR0_TS, %rdx
	mov	%rdx, %cr0   
	jmp	2f

1:	testl	$Thread_fpu_owner, OFS__THREAD__STATE (DEST_rsi)
	jz	2f
	// clear ts
	clts       

2:	// %eax=thread_state (THIS_ebx)
	xor	%edx, %edx

	// if (state() & Thread_ready && ! in_ready_list())
	//   ready_enqueue()
	cmp	%rdx, OFS__THREAD__READY_NEXT (THIS_rbx)
	jne	se_no_enqueue
	testb	$Thread_ready, %al
	jnz	se_enqueue_this

	.align	8
se_no_enqueue:
	// not for performance kernels!
	CONSUME_TIME 		// scratches eax, ecx, edx, ebp

	// push restart address onto old stack

//	mov	$se_ret_switch, %rax
	lea	se_ret_switch(%rip), %rax
	pushq	%rax

	mov	REGS_rsp, OFS__THREAD__KERNEL_SP (THIS_rbx)
#undef REGS_rsp
#define DEST_KERNEL_SP_rbp %rbp
	mov	OFS__THREAD__KERNEL_SP (DEST_rsi), DEST_KERNEL_SP_rbp

	// switch esp0 on TSS
	mov	CPUS_BASE + OFS__CPU__TSS, %rax
	leaq	THREAD_BLOCK_SIZE (DEST_rsi), %rcx

	mov	%rcx, 4 (%rax)	// x86_tss.esp0

	// we dont clear the IPC window of the destination thread
	// reason: ipc already finished and the destination thread
	// will setup an new one before entering long IPC again

//	int3
	// pdir = space_context - kmem::mem_phys (needed later)
	movq	OFS__THREAD__SPACE (DEST_rsi), %rax
	leaq	OFS__SPACE__MEM_SPACE (%rax), %rax
	sub	PHYSMEM_OFFS, %rax

	mov	PAGE_DIR_ADDR, %rcx		// get_pdir()
	cmp	%rax, %rcx			// get_pdir == pdir
	jne	se_flush_pdir			// no => flush

se_addr_space_switched:	

	lea	se_ret_switch(%rip), %rax
	cmp	%rax, (DEST_KERNEL_SP_rbp)
	jne	se_slow_switch

	RESET_THREAD_IPC_MASK_AT DEST_rsi

	// Setup return registers. We have to add 8 to each %rsp,%rbp reference
	// since there is the return address pushed on the stack.
	
	mov	OFS__THREAD__EIP (DEST_rsi), %rcx
        /* make RIP canonical, workaround for intel IA32e flaw */
        shl     $16, %rcx
        sar     $16, %rcx
	mov	OFS__THREAD__ESP (DEST_rsi), %r15
	mov	OFS__THREAD__EFL (DEST_rsi), %r11	
	mov	OFS__THREAD__ID  (THIS_rbx), %rsi

	mov	8+REG_RDX (%rsp), %rdx
	mov	8+REG_RBX (%rsp), %rbx
	mov	$RETURN_DOPE, %eax	

	mov	8+REG_R14 (%rbp), %r14
	mov	8+REG_R13 (%rbp), %r13
	mov	8+REG_R12 (%rbp), %r12
	mov	8+REG_R10 (%rbp), %r10
	mov	8+REG_R9 (%rbp), %r9
	mov	8+REG_R8 (%rbp), %r8
	
	mov	%r15, %rsp
	sysretq
	
se_flush_pdir:
	CNT_ADDR_SPACE_SWITCH
	mov	%rax, PAGE_DIR_ADDR			// set pdir, flush TLBs
	jmp	se_addr_space_switched

se_enqueue_this:
	// ecx = sched(), eax = sched()->prio()
	mov	OFS__THREAD__SCHED (THIS_rbx), %rcx

	xor	%eax, %eax
	movl	OFS__SCHED_CONTEXT__PRIO (%rcx), %eax
	READY_ENQUEUE THIS_rbx, se_no_enqueue // scratches ecx, edx
	jmp	se_no_enqueue

	.align	16
se_ret_switch:
	// shortcut success
	ESP_TO_TCB_AT %rbx
	RESET_THREAD_IPC_MASK_AT %rbx
	DO_SYSEXIT


	// The destination thread is not in a shortcut IPC so we cannot
	// throw it directly into user space since it may held a thread
	// lock or does not return via sysexit (int-entered IPC or
	// ex_regs manipulation)
	.align	16
se_slow_switch:
	mov	OFS__THREAD__RCV_REGS (DEST_rsi), %rax
	mov	8+REG_RDX (%rsp), %rdx
	mov	8+REG_RBX (%rsp), %rcx
	mov	%rdx, REG_RDX (%rax)		// dst_regs->edx = dw1
	mov	%rcx, REG_RBX (%rax)		// dst_regs->ebx = dw2
	mov	OFS__THREAD__ID (THIS_rbx), %rdx
	movq	$RETURN_DOPE, REG_RAX (%rax)
	mov	%rdx, REG_RSI (%rax)	        // dst_regs->esi = id.low
	mov	%rbp, %rsp			// load new stack pointer
	pop	%rax
	jmp	*%rax

	.globl	in_slow_ipc2
	.globl	se_ret_switch

#endif // CONFIG_PF_UX

	.globl	in_slow_ipc1
	.globl	i30_ret_switch

#endif // CONFIG_ASSEMBLER_IPC_SHORTCUT

	// fast return from Dirq::hit
	.align	16
	.globl	fast_ret_from_irq
fast_ret_from_irq:
	RESTORE_STATE
	popq	%rax
	andq	$0x7f, 8(%rsp)			// if entered using syscall
	orq	$EFLAGS_IF, 16(%rsp)		// if entered using syscall
	iretq

