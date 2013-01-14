INTERFACE:

#include "atomic.h"

template< bool LARGE >
class Bitmap_base;

// Implementation for bitmaps bigger than sizeof(unsigned long) * 8
// Derived classes have to make sure to provide the storage space for
// holding the bitmap.
template<>
class Bitmap_base< true >
{
public:
  void bit(unsigned long bit, bool on)
  {
    unsigned long idx = bit / Bpl;
    unsigned long b   = bit % Bpl;
    _bits()[idx] = (_bits()[idx] & ~(1UL << b)) | ((unsigned long)on << b);
  }

  void clear_bit(unsigned long bit)
  {
    unsigned long idx = bit / Bpl;
    unsigned long b   = bit % Bpl;
    _bits()[idx] &= ~(1UL << b);
  }

  void set_bit(unsigned long bit)
  {
    unsigned long idx = bit / Bpl;
    unsigned long b   = bit % Bpl;
    _bits()[idx] |= (1UL << b);
  }

  unsigned long operator [] (unsigned long bit) const
  {
    unsigned long idx = bit / Bpl;
    unsigned long b   = bit % Bpl;
    return _bits()[idx] & (1UL << b);
  }

  bool atomic_get_and_clear(unsigned long bit)
  {
    unsigned long idx = bit / Bpl;
    unsigned long b   = bit % Bpl;
    unsigned long v;
    do
      {
        v = _bits()[idx];
      }
    while (!mp_cas(&_bits()[idx], v, v & ~(1UL << b)));

    return v & (1UL << b);
  }

  void atomic_set_bit(unsigned long bit)
  {
    unsigned long idx = bit / Bpl;
    unsigned long b   = bit % Bpl;
    atomic_mp_or(&_bits()[idx], 1UL << b);
  }

  void atomic_clear_bit(unsigned long bit)
  {
    unsigned long idx = bit / Bpl;
    unsigned long b   = bit % Bpl;
    atomic_mp_and(&_bits()[idx], ~(1UL << b));
  }

protected:
  enum
  {
    Bpl      = sizeof(unsigned long) * 8,
  };

  Bitmap_base() {}
  Bitmap_base(Bitmap_base const &) = delete;
  Bitmap_base &operator = (Bitmap_base const &) = delete;

private:
  unsigned long *_bits()
  { return reinterpret_cast<unsigned long *>(this); }

  unsigned long const *_bits() const
  { return reinterpret_cast<unsigned long const *>(this); }
};

// Implementation for a bitmap up to sizeof(unsigned long) * 8 bits
template<>
class Bitmap_base<false>
{
public:
  void bit(unsigned long bit, bool on)
  {
    _bits[0] = (_bits[0] & ~(1UL << bit)) | ((unsigned long)on << bit);
  }

  void clear_bit(unsigned long bit)
  {
    _bits[0] &= ~(1UL << bit);
  }

  void set_bit(unsigned long bit)
  {
    _bits[0] |= 1UL << bit;
  }

  unsigned long operator [] (unsigned long bit) const
  {
    return _bits[0] & (1UL << bit);
  }

  bool atomic_get_and_clear(unsigned long bit)
  {
    unsigned long v;
    do
      {
        v = _bits[0];
      }
    while (!mp_cas(&_bits[0], v, v & ~(1UL << bit)));

    return v & (1UL << bit);
  }

  void atomic_set_bit(unsigned long bit)
  {
    atomic_mp_or(&_bits[0], 1UL << bit);
  }

  void atomic_clear_bit(unsigned long bit)
  {
    atomic_mp_and(&_bits[0], ~(1UL << bit));
  }

protected:
  enum
  {
    Bpl      = sizeof(unsigned long) * 8,
  };
  unsigned long _bits[1];

  Bitmap_base() {}
  Bitmap_base(Bitmap_base const &) = delete;
  Bitmap_base &operator = (Bitmap_base const &) = delete;
};

template<int BITS>
class Bitmap : public Bitmap_base< (BITS > sizeof(unsigned long) * 8) >
{
public:
  void clear_all()
  {
    for (unsigned i = 0; i < Nr_elems; ++i)
      _bits[i] = 0;
  }

  bool is_empty() const
  {
    for (unsigned i = 0; i < Nr_elems; ++i)
      if (_bits[i])
	return false;
    return true;
  }

  Bitmap() {}
  Bitmap(Bitmap const &o)
  { __builtin_memcpy(_bits, o._bits, sizeof(_bits)); }

  Bitmap &operator = (Bitmap const &o)
  { __builtin_memcpy(_bits, o._bits, sizeof(_bits)); return *this; }

private:
  enum {
    Bpl      = sizeof(unsigned long) * 8,
    Nr_elems = (BITS + Bpl - 1) / Bpl,
  };
  unsigned long _bits[Nr_elems];
};
