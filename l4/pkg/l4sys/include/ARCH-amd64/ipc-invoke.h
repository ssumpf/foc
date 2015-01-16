/**
 * \file
 * \brief   L4 IPC System Call Invoking in Assembler.
 * \ingroup api_calls
 *
 * This file can also be used in asm-files, so don't include C statements.
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
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
#if 0
/*
 * Some words about the sysenter entry frame: Since the sysenter instruction
 * automatically reloads the instruction pointer (eip) and the stack pointer
 * (esp) after kernel entry, we have to save both registers preliminary to
 * that instruction. We use ecx to store the user-level esp and save eip onto
 * the stack. The ecx register contains the IPC timeout and has to be saved
 * onto the stack, too. The ebp register is saved for compatibility reasons
 * with the Hazelnut kernel. Both the esp and the ss register are also pushed
 * onto the stack to be able to return using the "lret" instruction from the
 * sysexit trampoline page if Small Address Spaces are enabled.
 */

# ifndef L4F_IPC_SYSENTER

/** Kernel entry code for inline assembly. \internal */
#  define IPC_SYSENTER       "int  $0x30              \n\t"
/** Kernel entry code for assembler code. \internal */
#  define IPC_SYSENTER_ASM    int  $0x30;

# else
# error No pushs allowed because of red-zone
#  ifdef __PIC__
# error no PIC support for AMD64
#  else
#   define IPC_SYSENTER            \
     "push   %%rcx           \n\t"  \
     "push   %%r11	     \n\t"  \
     "push   %%r15	     \n\t"  \
     "syscall                \n\t"  \
     "pop    %%r15	     \n\t"  \
     "pop    %%r11	     \n\t"  \
     "pop    %%rcx	     \n\t"  \
     "0:                     \n\t"
#   define IPC_SYSENTER_ASM	 \
     push    %rcx		; \
     push    %r11		; \
     push    %r15		; \
     syscall			; \
     pop    %r15		; \
     pop    %r11		; \
     pop    %rcx		; \
     0:
#  endif

# endif

/** Kernel entry code for inline assembly. \internal */
#define L4_ENTER_KERNEL IPC_SYSENTER

#endif

