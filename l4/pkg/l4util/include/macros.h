/**
 * \file
 * \brief Utility macros.
 */
/*
 * (c) 2000-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *               Lars Reuther <reuther@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */
/*****************************************************************************/
/*****************************************************************************/
#ifndef _L4UTIL_MACROS_H
#define _L4UTIL_MACROS_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>
#include <l4/util/l4_macros.h>

/*****************************************************************************
 *** generic macros
 *****************************************************************************/

#ifdef ___________MOVED_TO_LOG_PKG

/* print message and enter kernel debugger */
#ifndef Panic

// Don't include <stdlib.h> here, leads to trouble.
// Don't use exit() here since we want to terminate ASAP.
// We might be executed in context of the region manager.
EXTERN_C_BEGIN
void _exit(int status) __attribute__ ((__noreturn__));
EXTERN_C_END

# ifdef L4BID_RELEASE_MODE
#  define Panic(args...) do                                      \
                           {                                     \
			     LOG(args);                          \
			     LOG_flush();                        \
			     _exit(-1);                          \
			   }                                     \
                         while (1)
# else
#  define Panic(args...) do                                      \
                           {                                     \
                             LOG(args);                          \
                             LOG_flush();                        \
                             enter_kdebug("PANIC, 'g' for exit");\
                             _exit(-1);                          \
                           }                                     \
                         while (1)
# endif
#endif

/* assertion */
#ifndef Assert
#  define Assert(expr) do                                        \
                         {                                       \
                           if (!(expr))                          \
                             {                                   \
                               LOG_printf(#expr "\n");           \
                               Panic("Assertion failed");        \
                             }                                   \
                         }                                       \
                       while (0)
#endif

/* enter kernel debugger */
#ifndef Kdebug
#  define Kdebug(args...)  do                                    \
                             {                                   \
                               LOG(args);                        \
                               LOG_flush();                      \
                               enter_kdebug("KD");               \
                             }                                   \
                           while (0)
#endif
#endif

/*****************************************************************************
 *** debug stuff (to be removed, use LOG* macros instead!)
 *****************************************************************************/

/* we use our own debug macros */
#ifdef KDEBUG
#  undef KDEBUG
#endif
#ifdef ASSERT
#  undef ASSERT
#endif
#ifdef PANIC
#  undef PANIC
#endif

#ifdef DEBUG

#define KDEBUG(args...) do                                   \
                          {                                  \
                            LOGl(args);                      \
                            LOG_flush();                     \
                            enter_kdebug("KD");              \
                          }                                  \
                        while (0)

#ifdef DEBUG_ASSERTIONS
#  define ASSERT(expr)    Assert(expr)
#else
#  define ASSERT(expr)    do {} while (0)
#endif

#ifdef DEBUG_ERRORS
#  define PANIC(format, args...) Panic(format, ## args)
#else
#  define PANIC(args...)  do {} while (0)
#endif

#else /* !DEBUG */

#define KDEBUG(args...) do {} while (0)
#define ASSERT(expr)    do {} while (0)
#define PANIC(args...)  do {} while (0)

#endif /* !DEBUG */

#endif /* !_L4UTIL_MACROS_H */
