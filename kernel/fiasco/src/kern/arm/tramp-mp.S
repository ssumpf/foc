#include "globalconfig.h"
#include "tcboffset.h"

	.section .mp_tramp, "awx"
	.p2align 12

#ifdef CONFIG_ARM_V7
	/* Directly taken from the ARMV7 manual section 'Performing cache
	   maintenance operations' */
	.global v7_invalidate_dcache
v7_invalidate_dcache:
	mrc p15, 1, r0, c0, c0, 1  @Read CLIDR into R0
	ands r3, r0, #0x07000000
	mov r3, r3, lsr #23        @ Cache level value (naturally aligned)
	beq finished
	mov r10, #0
loop1:
	add r2, r10, r10, lsr #1    @ Work out 3 x cachelevel
	mov r1, r0, lsr r2          @ bottom 3 bits are the Cache type for this level
	and r1, r1, #7              @ get those 3 bits alone
	cmp r1, #2
	blt skip                    @ no cache or only instruction cache at this level
	mcr p15, 2, r10, c0, c0, 0  @ write csselr from r10
	isb                         @ ISB to sync the change to the CCSIDR
	mrc p15, 1, r1, c0, c0, 0   @ read current CCSIDR to R1
	
	and r2, r1, #7              @ extract the line length field
	add r2, r2, #4              @ add 4 for the line length offset (log2 16 bytes)
	ldr r4, =0x3ff
	ands r4, r4, r1, lsr #3     @ R4 is the max number on the way size (right aligned)
	clz r5, r4                  @ R5 is the bit position of the way size increment
	mov r9, r4                  @ R9 working copy of the max way size (right aligned)
loop2:
	ldr r7, =0x00007fff
	ands r7, r7, r1, lsr #13    @ R7 is the max number of the index size (right aligned)
loop3:
	orr r11, r10, r9, lsl r5    @ factor in the way number and cache number into R11
	orr r11, r11, r7, lsl r2    @ factor in the index number
	mcr p15, 0, r11, c7, c14, 2 @ dccsw, clean/invalidate by set/way
	subs r7, r7, #1             @ decrement the index
	bge loop3

	subs r9, r9, #1             @ decrement the way number
	bge loop2

skip:
	add r10, r10, #2            @ increment the cache number
	cmp r3, r10
	bgt loop1

	mov     r3, #0
	mcr     p15, 2, r3, c0, c0, 0

	dsb
	isb
finished:
	mov pc, lr
#endif

	.global _tramp_mp_entry
_tramp_mp_entry:
	// IRQs off, SVC
	mrs   r0, cpsr
	orr   r0, #0xd3
	msr   cpsr_c, r0

	// enable SMP
#ifndef CONFIG_ARM_CORTEX_A15
	adr   r0, .Lmpcore_phys_base
	ldr   r0, [r0]
	ldr   r1, [r0]
	orr   r1, #1
	str   r1, [r0]
#endif

#ifdef CONFIG_ARM_V7
	bl    v7_invalidate_dcache
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
	adr   r3, .Lactrl_cpuid      // load addr
	ldm   r3, {r1,r2}            // load mask + val
	and   r0, r1                 // apply mask
	teq   r0, r2                 // check value
	bne   2f                     // only do mcr on this CPU
#endif

	mrc   p15, 0, r0, c1, c0, 1
#ifdef CONFIG_ARM_CORTEX_A15
	orr  r0, r0, #0x40
#elif defined CONFIG_ARM_V7
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

#ifdef CONFIG_ARM_CORTEX_A9
.Lactrl_cpuid:
         .long 0xff0ffff0
         .long 0x410fc090
#elif defined CONFIG_ARM_CORTEX_A15
.Lactrl_cpuid:
         .long 0xff0ffff0
         .long 0x410fc0f0
#else
.Lactrl_cpuid:
         .long 0xffffffff
         .long 0x0
#endif


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

