#ifndef CONFIG_TCBSIZE_H
#define CONFIG_TCBSIZE_H

#include <globalconfig.h>

#ifdef CONFIG_CONTEXT_4K
#define THREAD_BLOCK_SIZE (0x1000)
#else
#define THREAD_BLOCK_SIZE (0x800)
#endif

#endif
