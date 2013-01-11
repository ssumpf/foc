INTERFACE:

#include "paging.h"

class Pte;
class Mem_page_attr;

class Page_table //: public Page_table_defs
{
public:

  enum Status {
    E_OK = 0,
    E_NOMEM,
    E_EXISTS,
    E_UPGRADE,
    E_INVALID,
  };

#if 0
  void * operator new(size_t) throw();
  void operator delete(void *);
#endif
  static void init();

  Page_table();

  void copy_in(void *my_base, Page_table *o,
	       void *base, size_t size = 0, unsigned long asid = ~0UL);

  void *dir() const;

  static size_t num_page_sizes();
  static size_t const *page_sizes();
  static size_t const *page_shifts();
};

