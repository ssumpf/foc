INTERFACE:

#include <cxx/dlist>

class Queue;

class Queue_item : public cxx::D_list_item
{
  friend class Queue;
public:
  enum Status { Ok, Retry, Invalid };

private:
  Queue *_q;
} __attribute__((aligned(16)));


//--------------------------------------------------------------------------
IMPLEMENTATION:

#include "kdb_ke.h"
#include "std_macros.h"

PUBLIC inline
bool
Queue_item::queued() const
{ return cxx::D_list_cyclic<Queue_item>::in_list(this); }

PUBLIC inline NEEDS["kdb_ke.h"]
Queue *
Queue_item::queue() const
{
  assert_kdb (queued());
  return _q;
}

PUBLIC inline NEEDS["kdb_ke.h"]
Queue_item::Status
Queue_item::status() const
{
  assert_kdb (!queued());
  return Status((unsigned long)_q);
}

