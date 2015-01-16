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
#include <l4/log/macros.h>

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

#ifndef NDEBUG

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

#else /* NDEBUG */

#define KDEBUG(args...) do {} while (0)
#define ASSERT(expr)    do {} while (0)
#define PANIC(args...)  do {} while (0)

#endif /* NDEBUG */

#endif /* !_L4UTIL_MACROS_H */
