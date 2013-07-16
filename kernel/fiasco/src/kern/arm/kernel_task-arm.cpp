IMPLEMENTATION[arm]:

#include "config.h"
#include "globals.h"
#include "kmem_space.h"

PRIVATE inline NEEDS["globals.h", "kmem_space.h"]
Kernel_task::Kernel_task()
: Task(Ram_quota::root, Kmem_space::kdir(), Caps::none())
{}

