INTERFACE:

#include "types.h"

class Pci
{
  enum
  {
    Cfg_addr = 0xcf8,
    Cfg_data = 0xcfc,
  };
};


IMPLEMENTATION:

#include "io.h"

PUBLIC static inline NEEDS["io.h"]
Unsigned8
Pci::read_cfg8 (Mword bus, Mword dev, Mword subdev, Mword reg)
{
  Io::out32 (((bus    & 0xffff) << 16) |
             ((dev    &   0x1f) << 11) |
	     ((subdev &   0x07) <<  8) |
	     ((reg    &   0xff)      ) |
	     1<<31, Cfg_addr);
  return Io::in8 (Cfg_data + (reg & 3));
}

PUBLIC static inline NEEDS["io.h"]
Unsigned16
Pci::read_cfg16 (Mword bus, Mword dev, Mword subdev, Mword reg)
{
  Io::out32 (((bus    & 0xffff) << 16) |
             ((dev    &   0x1f) << 11) |
	     ((subdev &   0x07) <<  8) |
	     ((reg    &   0xfe)      ) |
	     1<<31, Cfg_addr);
  return Io::in16 (Cfg_data + (reg & 2));
}

PUBLIC static inline NEEDS["io.h"]
Unsigned32
Pci::read_cfg32 (Mword bus, Mword dev, Mword subdev, Mword reg)
{
  Io::out32 (((bus    & 0xffff) << 16) |
             ((dev    &   0x1f) << 11) |
	     ((subdev &   0x07) <<  8) |
	     ((reg    &   0xfc)      ) |
	     1<<31, Cfg_addr);
  return Io::in32 (Cfg_data);
}

PUBLIC static inline NEEDS["io.h"]
void
Pci::write_cfg8 (Mword bus, Mword dev, Mword subdev, Mword reg, Unsigned8 v)
{
  Io::out32 (((bus    & 0xffff) << 16) |
             ((dev    &   0x1f) << 11) |
	     ((subdev &   0x07) <<  8) |
	     ((reg    &   0xff)      ) |
	     1<<31, Cfg_addr);
  Io::out8 (v, Cfg_data + (reg & 3));
}

PUBLIC static inline NEEDS["io.h"]
void
Pci::write_cfg16 (Mword bus, Mword dev, Mword subdev, Mword reg, Unsigned16 v)
{
  Io::out32 (((bus    & 0xffff) << 16) |
             ((dev    &   0x1f) << 11) |
	     ((subdev &   0x07) <<  8) |
	     ((reg    &   0xfe)      ) |
	     1<<31, Cfg_addr);
  Io::out16 (v, Cfg_data + (reg & 2));
}

PUBLIC static inline NEEDS["io.h"]
void
Pci::write_cfg32 (Mword bus, Mword dev, Mword subdev, Mword reg, Unsigned32 v)
{
  Io::out32 (((bus    & 0xffff) << 16) |
             ((dev    &   0x1f) << 11) |
	     ((subdev &   0x07) <<  8) |
	     ((reg    &   0xfc)      ) |
	     1<<31, Cfg_addr);
  Io::out32 (v, Cfg_data);
}
