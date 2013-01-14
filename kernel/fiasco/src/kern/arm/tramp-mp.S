#include "globalconfig.h"
#include "tcboffset.h"

	.section .mp_tramp, "awx"
	.p2align 12

#ifdef CONFIG_ARM_V7
/* See cache_func_gen.cpp */
	.global v7_invalidate_l1
invalidate_l1_v7:
	mov     r3, #0
	mcr     p15, 2, r3, c0, c0, 0
	mrc     p15, 1, r2, c0, c0, 0
	mov     r3, r2, lsr #3
	mov     r0, r2, lsr #13
	mov     r3, r3, asl #22
	mov     r3, r3, lsr #22
	and     r2, r2, #7
	mov     r0, r0, asl #17
	add     ip, r2, #4
	mov     r0, r0, lsr #17
	mov     r2, r3
	clz     r3, r3
.L3:
	mov     r4, r2, asl r3
	mov     r1, r0
.L2:
	orr     r5, r4, r1, asl ip
	mcr     p15, 0, r5, c7, c6, 2
	subs    r1, r1, #1
	bcs     .L2
	subs    r2, r2, #1
	bcs     .L3
	isb
	dsb
	mov     pc, lr
#endif

	.global _tramp_mp_entry
_tramp_mp_entry:
	// IRQs off, SVC
	mrs   r0, cpsr
	orr   r0, #0xd3
	msr   cpsr_c, r0

	// enable SMP
	adr   r0, .Lmpcore_phys_base
	ldr   r0, [r0]
	ldr   r1, [r0]
	orr   r1, #1
	str   r1, [r0]

#ifdef CONFIG_ARM_V7
	bl    invalidate_l1_v7
#endif

	mcr   p15, 0, r0, c7, c5, 0 // ICIALLU
	mcr   p15, 0, r0, c7, c5, 6 // BPIALL

	mcr   p15, 0, r0, c7, c10, 4 // dsb
#ifdef CONFIG_ARM_V6
	mcr   p15, 0, r0, c7, c7, 0  // inv both
#endif

#ifdef CONFIG_ARM_V7
	// ACTRL is implementation defined
	mrc   p15, 0, r0, c0, c0, 0  // read MIDR
	adr   r3, .Lactrl_cpuid_a9   // load addr
	ldm   r3, {r1,r2}            // load mask + val
	and   r0, r1                 // apply mask
	teq   r0, r2                 // check value
	bne   2f                     // only do mcr on this CPU
#endif

	mrc   p15, 0, r0, c1, c0, 1
#ifdef CONFIG_ARM_V7
        tst   r0, #0x40
        bne   2f
	orr   r0, r0, #0x41
#else
	orr   r0, r0, #0x20
#endif
	mcr   p15, 0, r0, c1, c0, 1

2:

	// TLB flush
	mcr p15, 0, r0, c8, c7, 0

	adr r0, _tramp_mp_start_dcr
	ldr r0, [r0]
	mcr p15, 0, r0, c3, c0

	// init TTBCR
	mov r0, #0
	mcr p15, 0, r0, c2, c0, 2

	adr r0, _tramp_mp_startup_pdbr
	ldr r0, [r0]
	mcr p15, 0, r0, c2, c0

	adr r0, _tramp_mp_startup_cp15_c1
	ldr r0, [r0]
	mcr p15, 0, r0, c1, c0

	// barrier
	mrc p15, 0, r0, c2, c0, 0
	mov r0, r0
	ldr r0, 1f
	mov pc, r0
1:
	.long _tramp_mp_virt

.Lmpcore_phys_base:
	.long MPCORE_PHYS_BASE

// only one currently
.Lactrl_cpuid_a9:
         .long 0xff0ffff0
         .long 0x410fc090

	// we run paged now
_tramp_mp_virt:
	// spinlock on cpu-init
	adr	r0, _tramp_mp_spinlock
1:	ldr	r1, [r0]
	teq	r1, #0
	wfene
	bne	1b

2:	ldrex	r1, [r0]
	teq	r1, #0
	bne	1b
	mov	r1, #2
	strex	r2, r1, [r0]
	teq	r2, #0
	bne	1b

	// TLB flush
	mcr p15, 0, r0, c8, c7, 0

	adr	sp, _tramp_mp_init_stack_top
	nop
	nop
	nop
	ldr	pc, [pc, #-4]

	.long BOOT_AP_CPU

.global _tramp_mp_startup_cp15_c1
_tramp_mp_startup_cp15_c1:
	.long 0x00000000

.global _tramp_mp_startup_pdbr
_tramp_mp_startup_pdbr:
	.long 0x00000000

.global _tramp_mp_start_dcr
_tramp_mp_start_dcr:
	.long 0x00000000

.global _tramp_mp_spinlock
_tramp_mp_spinlock:
	.long 0

_tramp_mp_init_stack:
	.space 1024
_tramp_mp_init_stack_top:

