
#define GATE_INITTAB_BEGIN(name)	\
	.text	1			;\
	.globl	name			;\
name:					;\
	.text

#define	GATE_ENTRY(n,entry,type)	\
	.text	1			;\
	.quad	entry			;\
	.word	n			;\
	.word	type			;\
	.text

#define GATE_INITTAB_END		\
	.text	1			;\
	.quad	0			;\
	.text

#define	EXCEPTION(n,name)		\
	GATE_ENTRY(n,name,0x0e)		;\
name:					;\
	push	$(0)			;\
	push	$(n)			;\
	jmp	alltraps

#define	EXCEP_USR(n,name)		\
	GATE_ENTRY(n,name,0x6e)		;\
name:					;\
	push	$(0)			;\
	push	$(n)			;\
	jmp	alltraps

#define	EXCEP_ERR(n,name)		\
	GATE_ENTRY(n,name,0x0e)		;\
name:					;\
	push	$(n)			;\
	jmp	alltraps


GATE_INITTAB_BEGIN(boot_idt_inittab)

EXCEPTION(0x00,t_zero_div)
EXCEPTION(0x01,t_debug)
EXCEPTION(0x02,t_nmi)
EXCEP_USR(0x03,t_int3)
EXCEP_USR(0x04,t_into)
EXCEP_USR(0x05,t_bounds)
EXCEPTION(0x06,t_invop)
EXCEPTION(0x07,t_nofpu)
EXCEPTION(0x08,t_double_fault)
EXCEPTION(0x09,a_fpu_over)
EXCEP_ERR(0x0a,a_inv_tss)
EXCEP_ERR(0x0b,t_segnp)
EXCEP_ERR(0x0c,t_stack_fault)
EXCEP_ERR(0x0d,t_gen_prot)
EXCEP_ERR(0x0e,t_page_fault)
EXCEPTION(0x0f,t_trap_0f)
EXCEPTION(0x10,t_fpu_err)
EXCEPTION(0x11,t_trap_11)
EXCEPTION(0x12,t_trap_12)
EXCEPTION(0x13,t_trap_13)
EXCEPTION(0x14,t_trap_14)
EXCEPTION(0x15,t_trap_15)
EXCEPTION(0x16,t_trap_16)
EXCEPTION(0x17,t_trap_17)
EXCEPTION(0x18,t_trap_18)
EXCEPTION(0x19,t_trap_19)
EXCEPTION(0x1a,t_trap_1a)
EXCEPTION(0x1b,t_trap_1b)
EXCEPTION(0x1c,t_trap_1c)
EXCEPTION(0x1d,t_trap_1d)
EXCEPTION(0x1e,t_trap_1e)
EXCEPTION(0x1f,t_trap_1f)

GATE_INITTAB_END

alltraps:
	push	%r15
	push	%r14
	push	%r13
	push	%r12
	push	%r11
	push	%r10
	push	%r9
	push	%r8
	mov	%cr2,%r8
	push	%r8
	push	%rbp
	push	%rsi
	push	%rdi
	push	%rdx
	push	%rcx
	push	%rbx
	push	%rax	
	

	call	trap_dump_panic

