// vi:ft=cpp

#pragma once

#include "types.h"

class Mmio_register_block
{
public:
  Mmio_register_block() {}
  explicit Mmio_register_block(Address base) : _base(base) {}

  template< typename T >
  void write(T t, Address reg) const
  { *reinterpret_cast<T volatile *>(_base + reg) = t; }

  template< typename T >
  T read(Address reg) const
  { return *reinterpret_cast<T volatile const *>(_base + reg); }

  template< typename T >
  void modify(T enable, T disable, Address reg) const
  {
    Mword tmp = read<T>(reg);
    tmp &= ~disable;
    tmp |= enable;
    write<T>(tmp, reg);
  }

  template< typename T >
  void clear(T clear_bits, Address reg) const
  {
    modify<T>(0, clear_bits, reg);
  }

  template< typename T >
  void set(T set_bits, Address reg) const
  {
    modify<T>(set_bits, 0, reg);
  }

  Address get_mmio_base() const { return _base; }
  void set_mmio_base(Address base) { _base = base; }

private:
  Address _base;
};
