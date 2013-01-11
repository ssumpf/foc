/**
 * \file
 * \brief Map implementation
 */
/*
 * (c) 2008-2009 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *               Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU Lesser General Public License 2.1.
 * Please see the COPYING-LGPL-2.1 file for details.
 */
#ifndef SLMAP_H
#define SLMAP_H

#include <stdlib.h>

#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

/*
 * the map structure
 *
 * its basically a single linked list with a
 * key and a data entry
 */
typedef struct slmap_t
{
  struct slmap_t* next;    /* pointer to next entry */
  void *key;             /* the key of the map entry */
  unsigned key_size;     /* the size of the key entry */
  void *data;            /* the data of the map entry */
} slmap_t;

/*
 * function prototypes
 */

L4_CV slmap_t*
map_new_entry(void* key, unsigned key_size, void *data);

L4_CV slmap_t*
map_new_sentry(char* key, void* data);

L4_CV slmap_t*
map_append(slmap_t* list, slmap_t* new_entry);

L4_CV slmap_t*
map_remove(slmap_t* list, slmap_t* entry);

L4_CV void
map_free_entry(slmap_t** entry);

L4_CV slmap_t*
map_get_at(slmap_t* list, int n);

L4_CV slmap_t*
map_add(slmap_t* list, slmap_t* new_entry);

L4_CV void
map_insert_after(slmap_t* after, slmap_t* new_entry);

L4_CV slmap_t*
map_find(slmap_t* list, void* key, unsigned key_size);

L4_CV slmap_t*
map_sfind(slmap_t* list, const char* key);

/*
 * implementatic of static inline
 */

static inline unsigned char
map_is_empty(slmap_t* list)
{
  return (list) ? 0 : 1;
}

static inline void
map_free(slmap_t **list)
{
  while (*list)
    *list = map_remove(*list, *list);
}

static inline int
map_elements(slmap_t* list)
{
  register int n;
  for (n=0; list; list=list->next) 
      n++;
  return n;
}

EXTERN_C_END

#endif /* SLMAP_H */

