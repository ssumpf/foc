/**
 * \file
 * \brief Type information handling.
 */
/*
 * (c) 2010 Alexander Warg <warg@os.inf.tu-dresden.de>
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

#if defined(__GXX_RTTI) || !defined(L4_NO_RTTI)
#  include <typeinfo>
   typedef std::type_info const *L4_std_type_info_ptr;
#  define L4_KOBJECT_META_RTTI(type) (&typeid(type))
   inline char const *L4_kobject_type_name(L4_std_type_info_ptr n)
   { return n ? n->name() : 0; }
#else
   typedef void const *L4_std_type_info_ptr;
#  define L4_KOBJECT_META_RTTI(type) (0)
   inline char const *L4_kobject_type_name(L4_std_type_info_ptr)
   { return 0; }
#endif

namespace L4 {

/**
 * \brief Dynamic Type Information for L4Re Interfaces.
 *
 * This class represents the runtime-dynamic type information for
 * L4Re interfaces, and is not intended to be used directly by applications.
 * \note The interface of is subject to changes.
 *
 * The main use for this info is to be used by the implementation of the
 * L4::cap_dynamic_cast() function.
 *
 */
struct Type_info
{
  L4_std_type_info_ptr _type;
  Type_info const *const *_bases;
  unsigned _num_bases;
  long _proto;

  L4_std_type_info_ptr type() const { return _type; }
  Type_info const *base(unsigned idx) const { return _bases[idx]; }
  unsigned num_bases() const { return _num_bases; }
  long proto() const { return _proto; }
  char const *name() const { return L4_kobject_type_name(type()); }
  bool has_proto(long proto) const
  {
    if (_proto && _proto == proto)
      return true;

    if (!proto)
      return false;

    for (unsigned i = 0; i < _num_bases; ++i)
      if (base(i)->has_proto(proto))
	return true;

    return false;
  }
};


/**
 * \brief Get the L4::Type_info for the L4Re interface given in \a T.
 * \param T The type (L4Re interface) for which the information shall be
 *          returned.
 * \return A pointer to the L4::Type_info structure for \a T.
 */
template<typename T>
inline
Type_info const *kobject_typeid()
{ return &T::__Kobject_typeid::_m; }


/**
 * \internal
 */
#define L4____GEN_TI(t...)                             \
Type_info const t::__Kobject_typeid::_m =                                \
{                                                      \
  L4_KOBJECT_META_RTTI(Derived),                       \
  &t::__Kobject_typeid::_b[0], sizeof(t::__Kobject_typeid::_b) / sizeof(t::__Kobject_typeid::_b[0]), PROTO   \
}

/**
 * \internal
 */
#define L4____GEN_TI_MEMBERS()                                     \
private:                                                           \
  template< typename T > friend Type_info const *kobject_typeid(); \
protected: \
  struct __Kobject_typeid { \
  static Type_info const *const _b[];                              \
  static Type_info const _m;    };                                 \
public:                                                            \
  static long const Protocol = PROTO;


/**
 * \brief Helper class to create an L4Re interface class that is derived
 *        from a single base class.
 *
 * \param Derived is the name of the new interface.
 * \param Base is the name of the interfaces single base class.
 * \param PROTO may be set to the statically assigned protocol number
 *              used to communicate with this interface.
 *
 * The typical usage pattern is shown in the following code snippet. The
 * semantics of this example is an interface My_iface that is derived from
 * L4::Kobject.
 *
 * \code
 * class My_iface : public L4::Kobject_t<My_iface, L4::Kobject>
 * {
 *   ...
 * };
 * \endcode
 *
 */
template< typename Derived, typename Base, long PROTO = 0 >
class Kobject_t : public Base
{ L4____GEN_TI_MEMBERS() };


template< typename Derived, typename Base, long PROTO >
Type_info const *const Kobject_t<Derived, Base, PROTO>::__Kobject_typeid::_b[] =
{ &Base::__Kobject_typeid::_m };

/**
 * \internal
 */
template< typename Derived, typename Base, long PROTO >
L4____GEN_TI(Kobject_t<Derived, Base, PROTO>);


/**
 * \brief Helper class to create an L4Re interface class that is derived
 *        from two base classes.
 *
 * \param Derived is the name of the new interface.
 * \param Base1 is the name of the interfaces first base class.
 * \param Base2 is the name of the interfaces second base class.
 * \param PROTO may be set to the statically assigned protocol number
 *              used to communicate with this interface.
 *
 * The typical usage pattern is shown in the following code snippet. The
 * semantics of this example is an interface My_iface that is derived from
 * L4::Icu and L4Re::Dataspace.
 *
 * \code
 * class My_iface : public L4::Kobject_2t<My_iface, L4::Icu, L4Re::Dataspace>
 * {
 *   ...
 * };
 * \endcode
 *
 */
template< typename Derived, typename Base1, typename Base2, long PROTO = 0 >
class Kobject_2t : public Base1, public Base2
{ L4____GEN_TI_MEMBERS() };


template< typename Derived, typename Base1, typename Base2, long PROTO >
Type_info const *const  Kobject_2t<Derived, Base1, Base2, PROTO>::__Kobject_typeid::_b[] =
{
  &Base1::__Kobject_typeid::_m,
  &Base2::__Kobject_typeid::_m
};

/**
 * \internal
 */
template< typename Derived, typename Base1, typename Base2, long PROTO >
L4____GEN_TI(Kobject_2t<Derived, Base1, Base2, PROTO>);

#undef L4____GEN_TI
#undef L4____GEN_TI_MEMBERS

}
