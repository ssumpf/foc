/* -*- c -*- */
#include "config_gdt.h"
#include "tcboffset.h"
#include "linking.h"

#define OFFS(x) ((x)-(_tramp_mp_entry))
#define PHYS(x) (x)

#ifdef CONFIG_AMD64
# define CX rcx
# define SP rsp
#else
# define CX ecx
# define SP esp
#endif

#define CPU_NR_COUNTER (config_num_ap_cpus)

	.section .mp_tramp , "awx", @progbits
#ifndef CONFIG_PF_UX
	.code16
	.p2align 12
#endif
	.global _tramp_mp_entry
_tramp_mp_entry:

    /* Basic setup of the processor (stack not yet needed) */
#ifndef CONFIG_PF_UX
	cli
	cld
	movw	%cs, %ax
	movw	%ax, %ds

    /* Switch to protected mode (needed to read APIC-ID) */
	lgdtl	OFFS(_tramp_mp_init_gdt_pdesc)
	movl	%cr0, %eax
	orl	$0x00000001, %eax
	movl	%eax, %cr0
	ljmpl	$0x08, $PHYS(_tramp_mp_entry32)

	.code32
_tramp_mp_entry32:
	movw	$0x10, %ax
	movw	%ax, %ds

    /* Initialize paging, needs linear mapping of this page! (first 4MB are linear mapped, due to lucky cirumstances) */
	movl	PHYS(_tramp_mp_startup_pdbr), %eax
	movl	%eax, %cr3
	// Initialize cr4 before, because we may run on a 4MB-page and need the extension enabled before
	movl	PHYS(_tramp_mp_startup_cr4), %eax
	movl	%eax, %cr4

#ifdef CONFIG_AMD64
	mov %edx, %edi
	mov $0xc0000080, %ecx
	rdmsr
	or  $0x900,%eax // LME+NXE
	wrmsr
	mov %edi, %edx
	
	movl	PHYS(_tramp_mp_startup_cr0), %eax
	movl	%eax, %cr0

	ljmpl	$0x18, $PHYS(_tramp_mp_entry64)

	.code64
_tramp_mp_entry64:
	jmp	*PHYS(_ptr_64virt)

	.align 8
_ptr_64virt:
	.quad _entry_64virt

_entry_64virt:
#endif /* AMD64 */
    /* Reinitialize the gdt, reloads segments with kernel values */
	lgdt	PHYS(_tramp_mp_startup_gdt_pdesc)
#ifndef CONFIG_AMD64
	movl	PHYS(_tramp_mp_startup_cr0), %eax
	movl	%eax, %cr0
	ljmp	$GDT_CODE_KERNEL, $1f
1:	movw	$GDT_DATA_KERNEL, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
#endif /* ! AMD64 */
#endif /* ! UX */

    /* Increase CPU counter */
1:	mov	CPU_NR_COUNTER, %eax
	mov	%eax, %edi
	inc	%edi
	lock	; cmpxchg %edi, CPU_NR_COUNTER
	jnz	1b

    /* Acquire spinlock */
1:	cmpl $0, _tramp_mp_spinlock
	je 2f
	pause
	jmp 1b
2:	mov $2, %CX
	xchg _tramp_mp_spinlock, %CX
	cmp $0, %CX
	jne 1b

	/* we've the lock, can run on the init_stack */
	mov $_tramp_mp_init_stack_top, %SP
	mov %edi, %eax /* IA32: cpu-num in %eax, AMD64: %rdi */
	jmp BOOT_AP_CPU


#ifndef CONFIG_PF_UX
  /*
   * Pseudo_descriptor for the initial GDT
   */
   	.align 4
_tramp_mp_init_gdt_pdesc:
	.word 0x1f
	.long PHYS(_tramp_mp_init_gdt)
	.word 0

  /*
   * Initial GDT, used for switch to protected mode, will be
   * used only until startup sequence (from this point we use
   * the OS provided GDT).
   */
   	.align 8
_tramp_mp_init_gdt:
	.long 0x00000000, 0x00000000 /* dummy                */
	.long 0x0000FFFF, 0x00CF9A00 /* r-x/0..4GB/PL0/32bit */
	.long 0x0000FFFF, 0x00CF9200 /* rw-/0..4GB/PL0/32bit */
	.long 0x0000FFFF, 0x00AF9B00 /* r-x/0..4GB/PL0/64bit */

.global _tramp_mp_startup_cr0
_tramp_mp_startup_cr0:
	.quad 0x00000000

.global _tramp_mp_startup_cr4
_tramp_mp_startup_cr4:
	.quad 0x00000000

.global _tramp_mp_startup_pdbr
_tramp_mp_startup_pdbr:
	.quad 0x00000000

.global _tramp_mp_startup_gdt_pdesc
_tramp_mp_startup_gdt_pdesc:
	.quad 0
	.quad 0
#endif /* ! UX */

.global _tramp_mp_spinlock
_tramp_mp_spinlock:
	.quad 0

	.align 16
_tramp_mp_init_stack:
#ifdef CONFIG_PF_UX
	/* glibc *printf-functions */
	.space 4096
#else
	.space 2048
#endif
_tramp_mp_init_stack_top:

