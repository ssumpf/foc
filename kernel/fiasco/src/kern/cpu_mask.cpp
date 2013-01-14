INTERFACE:

#include "bitmap.h"
#include "config.h"

template<unsigned MAX_NUM_CPUS>
class Cpu_mask_t
{
public:
  enum { Max_num_cpus = MAX_NUM_CPUS };
  enum class Init { Bss };
  Cpu_mask_t(Init) {}
  Cpu_mask_t() { _b.clear_all(); }

  Cpu_mask_t(Cpu_mask_t const &) = default;
  Cpu_mask_t &operator = (Cpu_mask_t const &) = default;

  bool empty() const { return _b.is_empty(); }
  bool get(unsigned cpu) const { return _b[cpu]; }
  void clear(unsigned cpu) { return _b.clear_bit(cpu); }
  void set(unsigned cpu) { _b.set_bit(cpu); };
  void atomic_set(unsigned cpu) {_b.atomic_set_bit(cpu); }
  void atomic_clear(unsigned cpu) {_b.atomic_clear_bit(cpu); }
  bool atomic_get_and_clear(unsigned cpu)
  { return _b.atomic_get_and_clear(cpu); }

private:
  Bitmap<Max_num_cpus> _b;
};

typedef Cpu_mask_t<Config::Max_num_cpus> Cpu_mask;
