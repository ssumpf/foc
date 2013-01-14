/*****************************************************************************/
/**
 * \file
 * \brief   Kernel debugger macros
 * \ingroup api_calls
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Björn Döbel <doebel@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>
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
/*****************************************************************************/
#ifndef __L4_KDEBUG_H__
#define __L4_KDEBUG_H__

#include <l4/sys/compiler.h>

/**
 * Enter L4 kernel debugger
 * \ingroup l4_debugger_api
 * \hideinitializer
 *
 * \param   text         Text to be shown at kernel debugger prompt
 */
#ifndef __ASSEMBLER__
#define enter_kdebug(text) \
asm(\
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\"" text "\"\n\t"\
    "1:			\n\t"\
    )
#else
#define enter_kdebug(text)   \
    int	$3	      ;\
    jmp	1f	      ;\
    .ascii	text  ;\
    1:
#endif

/**
 * Enter L4 kernel debugger (plain assembler version)
 * \ingroup l4_debugger_api
 * \hideinitializer
 *
 * \param   text         Text to be shown at kernel debugger prompt
 */
#define asm_enter_kdebug(text) \
    "int	$3	\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\"" text "\"\n\t"\
    "1:			\n\t"

/**
 * Show message with L4 kernel debugger, but do not enter debugger
 * \ingroup l4_debugger_api
 * \hideinitializer
 *
 * \param   text         Text to be shown
 */
#define kd_display(text) \
asm(\
    "int	$3	\n\t"\
    "nop		\n\t"\
    "jmp	1f	\n\t"\
    ".ascii	\"" text "\"\n\t"\
    "1:			\n\t"\
    )

/**
 * Output character with L4 kernel debugger
 * \ingroup l4_debugger_api
 * \hideinitializer
 *
 * \param   c            Character to be shown
 */
#define ko(c) 					\
  asm(						\
      "int	$3	\n\t"			\
      "cmpb	%0,%%al	\n\t"			\
      : /* No output */				\
      : "N" (c)					\
      )

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

#ifndef __ASSEMBLER__

/**
 * Print character
 * \ingroup l4_debugger_api
 *
 * \param   c            Character
 */
L4_INLINE void
outchar(char c) L4_NOTHROW;

/**
 * Print character string
 * \ingroup l4_debugger_api
 *
 * \param   text         String
 */
L4_INLINE void
outstring(const char * text) L4_NOTHROW;

/**
 * Print character string
 * \ingroup l4_debugger_api
 *
 * \param   text         String
 * \param   len          Number of characters
 */
L4_INLINE void
outnstring(char const *text, unsigned len) L4_NOTHROW;

/**
 * Print 32 bit number (hexadecimal)
 * \ingroup l4_debugger_api
 *
 * \param   number       32 bit number
 */
L4_INLINE void
outhex32(int number) L4_NOTHROW;

/**
 * Print 20 bit number (hexadecimal)
 * \ingroup l4_debugger_api
 *
 * \param   number       20 bit number
 */
L4_INLINE void
outhex20(int number) L4_NOTHROW;

/**
 * Print 16 bit number (hexadecimal)
 * \ingroup l4_debugger_api
 *
 * \param   number       16 bit number
 */
L4_INLINE void
outhex16(int number) L4_NOTHROW;

/**
 * Print 12 bit number (hexadecimal)
 * \ingroup l4_debugger_api
 *
 * \param   number       12 bit number
 */
L4_INLINE void
outhex12(int number) L4_NOTHROW;

/**
 * Print 8 bit number (hexadecimal)
 * \ingroup l4_debugger_api
 *
 * \param   number       8 bit number
 */
L4_INLINE void
outhex8(int number) L4_NOTHROW;

/**
 * Print number (decimal)
 * \ingroup l4_debugger_api
 *
 * \param   number       Number
 */
L4_INLINE void
outdec(int number) L4_NOTHROW;

/**
 * Read character from console, non blocking
 * \ingroup l4_debugger_api
 *
 * \return Input character, -1 if no character to read
 */
L4_INLINE char
l4kd_inchar(void) L4_NOTHROW;

/**
 * Start profiling
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_profile_start(void) L4_NOTHROW;

/**
 * Stop profiling and dump result to console
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_profile_stop_and_dump(void) L4_NOTHROW;

/**
 * Stop profiling
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_profile_stop(void) L4_NOTHROW;

/**
 * Enable Fiasco watchdog
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_watchdog_enable(void) L4_NOTHROW;

/**
 * Disable Fiasco watchdog
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_watchdog_disable(void) L4_NOTHROW;

/**
 * Disable automatic resetting of watchdog. User is responsible to call
 * \c fiasco_watchdog_touch from time to time to ensure that the watchdog
 * does not trigger.
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_watchdog_takeover(void) L4_NOTHROW;

/**
 * Reenable automatic resetting of watchdog.
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_watchdog_giveback(void) L4_NOTHROW;

/**
 * Reset watchdog from user land. This function \b must be called from time
 * to time to prevent the watchdog from triggering if the watchdog is
 * activated and if \c fiasco_watchdog_takeover was performed.
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_watchdog_touch(void) L4_NOTHROW;


/*****************************************************************************
 *** Implementation
 *****************************************************************************/

L4_INLINE void
outchar(char c) L4_NOTHROW
{
  asm(
      "int	$3	\n\t"
      "cmpb	$0,%%al	\n\t"
      : /* No output */
      : "a" (c)
      );
}

/* actually outstring is outcstring */
L4_INLINE void
outstring(const char *text) L4_NOTHROW
{
  asm volatile (
      "int	$3	\n\t"
      "cmpb	$2,%%al \n\t"
      : /* No output */
      : "a" (text)
      : "memory"
      );
}

/* actually outstring is outcstring */
L4_INLINE void
outnstring(char const *text, unsigned len) L4_NOTHROW
{
  asm volatile (
      "pushl    %%ebx        \n\t"
      "movl     %%ecx, %%ebx \n\t"
      "int	$3	     \n\t"
      "cmpb	$1,%%al      \n\t"
      "popl     %%ebx        \n\t"
      : /* No output */
      : "a" (text), "c"(len)
      : "memory"
      );
}

L4_INLINE void
outhex32(int number) L4_NOTHROW
{
  asm(
      "int	$3	\n\t"
      "cmpb	$5,%%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void
outhex20(int number) L4_NOTHROW
{
  asm(
      "int	$3	\n\t"
      "cmpb	$6,%%al	\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void
outhex16(int number) L4_NOTHROW
{
  asm(
      "int	$3	\n\t"
      "cmpb	$7, %%al\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void
outhex12(int number) L4_NOTHROW
{
  asm(
      "int	$3	\n\t"
      "cmpb	$8, %%al\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void
outhex8(int number) L4_NOTHROW
{
  asm(
      "int	$3	\n\t"
      "cmpb	$9, %%al\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE void
outdec(int number) L4_NOTHROW
{
  asm(
      "int	$3	\n\t"
      "cmpb	$11, %%al\n\t"
      : /* No output */
      : "a" (number)
      );
}

L4_INLINE char
l4kd_inchar(void) L4_NOTHROW
{
  char c;
  asm volatile ("int $3; cmpb $13, %%al" : "=a" (c));
  return c;
}

L4_INLINE void
fiasco_profile_start(void) L4_NOTHROW
{
  asm("int $3; cmpb $24, %al");
}

L4_INLINE void
fiasco_profile_stop_and_dump(void) L4_NOTHROW
{
  asm("int $3; cmpb $25, %al");
}

L4_INLINE void
fiasco_profile_stop(void) L4_NOTHROW
{
  asm("int $3; cmpb $26, %al");
}

L4_INLINE void
fiasco_watchdog_enable(void) L4_NOTHROW
{
  asm("int $3; cmpb $31, %%al" : : "c" (1));
}

L4_INLINE void
fiasco_watchdog_disable(void) L4_NOTHROW
{
  asm("int $3; cmpb $31, %%al" : : "c" (2));
}

L4_INLINE void
fiasco_watchdog_takeover(void) L4_NOTHROW
{
  asm("int $3; cmpb $31, %%al" : : "c" (3));
}

L4_INLINE void
fiasco_watchdog_giveback(void) L4_NOTHROW
{
  asm("int $3; cmpb $31, %%al" : : "c" (4));
}

L4_INLINE void
fiasco_watchdog_touch(void) L4_NOTHROW
{
  asm("int $3; cmpb $31, %%al" : : "c" (5));
}

#endif /* __ASSEMBLER__ */

#endif /* __L4_KDEBUG_H__ */
