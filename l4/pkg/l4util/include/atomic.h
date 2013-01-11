/*****************************************************************************/
/**
 * \file
 * \brief   atomic operations header and generic implementations
 * \ingroup l4util_atomic
 *
 * \date    10/20/2000
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>,
 *          Jork Loeser  <jork@os.inf.tu-dresden.de> */
/*
 * (c) 2000-2009 Author(s)
 *     economic rights: Technische Universität Dresden (Germany)
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */

/*****************************************************************************/
#ifndef __L4UTIL__INCLUDE__ATOMIC_H__
#define __L4UTIL__INCLUDE__ATOMIC_H__

#include <l4/sys/l4int.h>
#include <l4/sys/compiler.h>

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

EXTERN_C_BEGIN

/** 
 * \defgroup l4util_atomic Atomic Instructions
 * \ingroup l4util_api
 */

/**
 * \brief Atomic compare and exchange (64 bit version)
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 * \param  cmp_val       compare value
 * \param  new_val       new value for dest
 *
 * \return 0 if comparison failed, 1 otherwise
 *
 * Compare the value in \em dest with \em cmp_val, if equal set \em dest to
 * \em new_val
 */
L4_INLINE int
l4util_cmpxchg64(volatile l4_uint64_t * dest,
                 l4_uint64_t cmp_val, l4_uint64_t new_val);

/**
 * \brief Atomic compare and exchange (32 bit version)
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 * \param  cmp_val       compare value
 * \param  new_val       new value for dest
 *
 * \return 0 if comparison failed, !=0 otherwise
 *
 * Compare the value in \em dest with \em cmp_val, if equal set \em dest to
 * \em new_val
 */
L4_INLINE int
l4util_cmpxchg32(volatile l4_uint32_t * dest,
                 l4_uint32_t cmp_val, l4_uint32_t new_val);

/**
 * \brief Atomic compare and exchange (16 bit version)
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 * \param  cmp_val       compare value
 * \param  new_val       new value for dest
 *
 * \return 0 if comparison failed, !=0 otherwise
 *
 * Compare the value in \em dest with \em cmp_val, if equal set \em dest to
 * \em new_val
 */
L4_INLINE int
l4util_cmpxchg16(volatile l4_uint16_t * dest,
                 l4_uint16_t cmp_val, l4_uint16_t new_val);

/**
 * \brief Atomic compare and exchange (8 bit version)
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 * \param  cmp_val       compare value
 * \param  new_val       new value for dest
 *
 * \return 0 if comparison failed, !=0 otherwise
 *
 * Compare the value in \em dest with \em cmp_val, if equal set \em dest to
 * \em new_val
 */
L4_INLINE int
l4util_cmpxchg8(volatile l4_uint8_t * dest,
                l4_uint8_t cmp_val, l4_uint8_t new_val);

/**
 * \brief Atomic compare and exchange (machine wide fields)
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 * \param  cmp_val       compare value
 * \param  new_val       new value for dest
 *
 * \return 0 if comparison failed, 1 otherwise
 *
 * Compare the value in \em dest with \em cmp_val, if equal set \em dest to
 * \em new_val
 */
L4_INLINE int
l4util_cmpxchg(volatile l4_umword_t * dest,
               l4_umword_t cmp_val, l4_umword_t new_val);

/**
 * \brief Atomic exchange (32 bit version)
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 * \param  val           new value for dest
 *
 * \return old value at destination
 */
L4_INLINE l4_uint32_t
l4util_xchg32(volatile l4_uint32_t * dest, l4_uint32_t val);

/**
 * \brief Atomic exchange (16 bit version)
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 * \param  val           new value for dest
 *
 * \return old value at destination
 */
L4_INLINE l4_uint16_t
l4util_xchg16(volatile l4_uint16_t * dest, l4_uint16_t val);

/**
 * \brief Atomic exchange (8 bit version)
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 * \param  val           new value for dest
 *
 * \return old value at destination
 */
L4_INLINE l4_uint8_t
l4util_xchg8(volatile l4_uint8_t * dest, l4_uint8_t val);

/**
 * \brief Atomic exchange (machine wide fields)
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 * \param  val           new value for dest
 *
 * \return old value at destination
 */
L4_INLINE l4_umword_t
l4util_xchg(volatile l4_umword_t * dest, l4_umword_t val);

//!@name Atomic add/sub/and/or (8,16,32 bit version) without result
/** @{
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 * \param  val           value to add/sub/and/or
 */
L4_INLINE void
l4util_add8(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE void
l4util_add16(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE void
l4util_add32(volatile l4_uint32_t *dest, l4_uint32_t val);
L4_INLINE void
l4util_sub8(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE void
l4util_sub16(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE void
l4util_sub32(volatile l4_uint32_t *dest, l4_uint32_t val);
L4_INLINE void
l4util_and8(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE void
l4util_and16(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE void
l4util_and32(volatile l4_uint32_t *dest, l4_uint32_t val);
L4_INLINE void
l4util_or8(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE void
l4util_or16(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE void
l4util_or32(volatile l4_uint32_t *dest, l4_uint32_t val);
//@}

//!@name Atomic add/sub/and/or operations (8,16,32 bit) with result
/** @{
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 * \param  val           value to add/sub/and/or
 * \return res
 */
L4_INLINE l4_uint8_t
l4util_add8_res(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE l4_uint16_t
l4util_add16_res(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE l4_uint32_t
l4util_add32_res(volatile l4_uint32_t *dest, l4_uint32_t val);
L4_INLINE l4_uint8_t
l4util_sub8_res(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE l4_uint16_t
l4util_sub16_res(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE l4_uint32_t
l4util_sub32_res(volatile l4_uint32_t *dest, l4_uint32_t val);
L4_INLINE l4_uint8_t
l4util_and8_res(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE l4_uint16_t
l4util_and16_res(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE l4_uint32_t
l4util_and32_res(volatile l4_uint32_t *dest, l4_uint32_t val);
L4_INLINE l4_uint8_t
l4util_or8_res(volatile l4_uint8_t *dest, l4_uint8_t val);
L4_INLINE l4_uint16_t
l4util_or16_res(volatile l4_uint16_t *dest, l4_uint16_t val);
L4_INLINE l4_uint32_t
l4util_or32_res(volatile l4_uint32_t *dest, l4_uint32_t val);
//@}

//!@name Atomic inc/dec (8,16,32 bit) without result
/** @{
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 */
L4_INLINE void
l4util_inc8(volatile l4_uint8_t *dest);
L4_INLINE void
l4util_inc16(volatile l4_uint16_t *dest);
L4_INLINE void
l4util_inc32(volatile l4_uint32_t *dest);
L4_INLINE void
l4util_dec8(volatile l4_uint8_t *dest);
L4_INLINE void
l4util_dec16(volatile l4_uint16_t *dest);
L4_INLINE void
l4util_dec32(volatile l4_uint32_t *dest);
//@}

//!@name Atomic inc/dec (8,16,32 bit) with result
/** @{
 * \ingroup l4util_atomic
 *
 * \param  dest          destination operand
 * \return res
 */
L4_INLINE l4_uint8_t
l4util_inc8_res(volatile l4_uint8_t *dest);
L4_INLINE l4_uint16_t
l4util_inc16_res(volatile l4_uint16_t *dest);
L4_INLINE l4_uint32_t
l4util_inc32_res(volatile l4_uint32_t *dest);
L4_INLINE l4_uint8_t
l4util_dec8_res(volatile l4_uint8_t *dest);
L4_INLINE l4_uint16_t
l4util_dec16_res(volatile l4_uint16_t *dest);
L4_INLINE l4_uint32_t
l4util_dec32_res(volatile l4_uint32_t *dest);
//@}

/**
 * \brief Atomic add
 * \ingroup l4util_atomic
 *
 * \param  dest      destination operand
 * \param  val       value to add
 */
L4_INLINE void
l4util_atomic_add(volatile long *dest, long val);

/**
 * \brief Atomic increment
 * \ingroup l4util_atomic
 *
 * \param  dest      destination operand
 */
L4_INLINE void
l4util_atomic_inc(volatile long *dest);

EXTERN_C_END

/*****************************************************************************
 *** Get architecture specific implementations
 *****************************************************************************/
#include <l4/util/atomic_arch.h>

#ifdef __GNUC__

/* Non-implemented version, catch with a linker warning */

extern int __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(void);

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_INC8
L4_INLINE void
l4util_inc8(volatile l4_uint8_t *dest)
{ (void)dest; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_INC16
L4_INLINE void
l4util_inc16(volatile l4_uint16_t *dest)
{ (void)dest; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_DEC8
L4_INLINE void
l4util_dec8(volatile l4_uint8_t *dest)
{ (void)dest; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_DEC16
L4_INLINE void
l4util_dec16(volatile l4_uint16_t *dest)
{ (void)dest; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif



#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_INC8_RES
L4_INLINE l4_uint8_t
l4util_inc8_res(volatile l4_uint8_t *dest)
{ (void)dest; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0; }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_INC16_RES
L4_INLINE l4_uint16_t
l4util_inc16_res(volatile l4_uint16_t *dest)
{ (void)dest; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0; }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_DEC8_RES
L4_INLINE l4_uint8_t
l4util_dec8_res(volatile l4_uint8_t *dest)
{ (void)dest; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0; }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_DEC16_RES
L4_INLINE l4_uint16_t
l4util_dec16_res(volatile l4_uint16_t *dest)
{ (void)dest; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0; }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG64
L4_INLINE int
l4util_cmpxchg64(volatile l4_uint64_t * dest,
                     l4_uint64_t cmp_val, l4_uint64_t new_val)
{ (void)dest; (void)cmp_val; (void)new_val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0; }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG8
L4_INLINE int
l4util_cmpxchg8(volatile l4_uint8_t * dest,
                    l4_uint8_t cmp_val, l4_uint8_t new_val)
{ (void)dest; (void)cmp_val; (void)new_val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0; }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG16
L4_INLINE int
l4util_cmpxchg16(volatile l4_uint16_t * dest,
                 l4_uint16_t cmp_val, l4_uint16_t new_val)
{ (void)dest; (void)cmp_val; (void)new_val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0; }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_CMPXCHG
L4_INLINE int
l4util_cmpxchg(volatile l4_umword_t * dest,
                   l4_umword_t cmp_val, l4_umword_t new_val)
{ (void)dest; (void)cmp_val; (void)new_val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0; }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_XCHG32
L4_INLINE l4_uint32_t
l4util_xchg32(volatile l4_uint32_t * dest, l4_uint32_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0; }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_XCHG16
L4_INLINE l4_uint16_t
l4util_xchg16(volatile l4_uint16_t * dest, l4_uint16_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0; }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_XCHG8
L4_INLINE l4_uint8_t
l4util_xchg8(volatile l4_uint8_t * dest, l4_uint8_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0; }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_XCHG
L4_INLINE l4_umword_t
l4util_xchg(volatile l4_umword_t * dest, l4_umword_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0; }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_ADD8
L4_INLINE void
l4util_add8(volatile l4_uint8_t *dest, l4_uint8_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_ADD16
L4_INLINE void
l4util_add16(volatile l4_uint16_t *dest, l4_uint16_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_ADD32
L4_INLINE void
l4util_add32(volatile l4_uint32_t *dest, l4_uint32_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif


#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_SUB8
L4_INLINE void
l4util_sub8(volatile l4_uint8_t *dest, l4_uint8_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_SUB16
L4_INLINE void
l4util_sub16(volatile l4_uint16_t *dest, l4_uint16_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_SUB32
L4_INLINE void
l4util_sub32(volatile l4_uint32_t *dest, l4_uint32_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_AND8
L4_INLINE void
l4util_and8(volatile l4_uint8_t *dest, l4_uint8_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_AND16
L4_INLINE void
l4util_and16(volatile l4_uint16_t *dest, l4_uint16_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_AND32
L4_INLINE void
l4util_and32(volatile l4_uint32_t *dest, l4_uint32_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_OR8
L4_INLINE void
l4util_or8(volatile l4_uint8_t *dest, l4_uint8_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_OR16
L4_INLINE void
l4util_or16(volatile l4_uint16_t *dest, l4_uint16_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_OR32
L4_INLINE void
l4util_or32(volatile l4_uint32_t *dest, l4_uint32_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); }
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_ADD8_RES
L4_INLINE l4_uint8_t
l4util_add8_res(volatile l4_uint8_t *dest, l4_uint8_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0;}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_ADD16_RES
L4_INLINE l4_uint16_t
l4util_add16_res(volatile l4_uint16_t *dest, l4_uint16_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0;}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_ADD32_RES
L4_INLINE l4_uint32_t
l4util_add32_res(volatile l4_uint32_t *dest, l4_uint32_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0;}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_SUB8_RES
L4_INLINE l4_uint8_t
l4util_sub8_res(volatile l4_uint8_t *dest, l4_uint8_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0;}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_SUB16_RES
L4_INLINE l4_uint16_t
l4util_sub16_res(volatile l4_uint16_t *dest, l4_uint16_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0;}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_SUB32_RES
L4_INLINE l4_uint32_t
l4util_sub32_res(volatile l4_uint32_t *dest, l4_uint32_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0;}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_AND8_RES
L4_INLINE l4_uint8_t
l4util_and8_res(volatile l4_uint8_t *dest, l4_uint8_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0;}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_AND16_RES
L4_INLINE l4_uint16_t
l4util_and16_res(volatile l4_uint16_t *dest, l4_uint16_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0;}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_AND32_RES
L4_INLINE l4_uint32_t
l4util_and32_res(volatile l4_uint32_t *dest, l4_uint32_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0;}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_OR8_RES
L4_INLINE l4_uint8_t
l4util_or8_res(volatile l4_uint8_t *dest, l4_uint8_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0;}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_OR16_RES
L4_INLINE l4_uint16_t
l4util_or16_res(volatile l4_uint16_t *dest, l4_uint16_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0;}
#endif

#ifndef __L4UTIL_ATOMIC_HAVE_ARCH_OR32_RES
L4_INLINE l4_uint32_t
l4util_or32_res(volatile l4_uint32_t *dest, l4_uint32_t val)
{ (void)dest; (void)val; __this_l4util_atomic_function_is_not_implemented_for_this_arch__sorry(); return 0;}
#endif


#endif //_GNUC__

#endif /* ! __L4UTIL__INCLUDE__ATOMIC_H__ */
