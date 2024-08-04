#ifndef CONTAINERS_H_
#define CONTAINERS_H_

#include "container_types.h"

#include <sys/mman.h>

#define MAX_ENTITY_COUNT 1024UL * 1024UL

#define bd_no_grow_slot(bd) ((bd)->items[(bd)->slot].data_type = OBJECT_ITEM, (bd)->slot - 1)
#define bd_grow_slot(bd) ((bd)->count++,                                        \
                             (bd)->items[(bd)->count - 1].generation = 0,          \
                             (bd)->items[(bd)->count - 1].data_type = OBJECT_ITEM, \
                             (bd)->count - 2)                                      

#define bulk_data_size(bd) ((bd)->count ? ((bd)->count - 1) : 0)
#define bulk_data_make_weak_ptr(bd, i) ((weak_pointer_t){(i), (bd)->items[(i) + 1].generation}) //create a weak reference
#define bulk_data_seti(bd, i, v) ((bd)->items[(i) + 1].data = (v))

#define bulk_data_get_wp(bd, wp) (((bd)->items[(wp).id + 1].generation == (wp).generation) ? (bulk_data_getp((bd),(wp).id)) : NULL)

#define bulk_data_delete_item(bd, i)                                \
do                                                                  \
{                                                                   \
    (bd)->items[i + 1].next      = (bd)->items[0].next;             \
    (bd)->items[i + 1].data_type = FREELIST_ITEM;                   \
    (bd)->items[i + 1].generation++;                                \
    (bd)->items[0].next          = i + 1;                           \
} while(0)  

#define bulk_data_push(bd) ((bd)->slot = (bd)->items[0].next,                                   \
                            (bd)->items[0].next = (bd)->items[(bd)->slot].next,                 \
                            (bd)->slot ? (bd_no_grow_slot((bd))) : (bd_grow_slot((bd)))) 
#define bulk_data_getp_raw(bd, i) (&(bd)->items[(i) + 1].data)

#define bulk_data_push_struct(bd) (bulk_data_getp_raw((bd), bulk_data_push((bd))))
#define bulk_data_push_value(bd, v) ((bd)->slot = bulk_data_push(&(bd)), bulk_data_seti((bd), (bd)->slot))

/**********************getters************************************ */
//return data if exists, return 0 or NULL if not
#define bulk_data_geti(bd, i) ((bd)->items[(i) + 1].data_type == OBJECT_ITEM ? (bd)->items[(i) + 1].data : 0)
#define bulk_data_getp_null(bd, i) ((bd)->items[(i) + 1].data_type == OBJECT_ITEM ? &(bd)->items[(i) + 1].data : NULL)

#define bulk_data_clear(bd)                             \
do                                                      \
{                                                       \
    for (uint32_t i = 1; i < (bd)->count; i++) \
    {                                                   \
        bulk_data_delete_item((bd), i);                 \
    }                                                   \
}while(0)

#define bulk_data_init(bd, type)                           \
do                                                         \
{                                                          \
memset((bd), 0, sizeof(*(bd)));                            \
item_##type     dummy;                                     \
dummy.next = 0;                                            \
dummy.generation = 0;                                      \
(bd)->items = mmap(0,                                      \
                   MAX_ENTITY_COUNT * sizeof(type),        \
                   PROT_READ | PROT_WRITE,                 \
                   MAP_PRIVATE | MAP_ANONYMOUS,            \
                   -1,                                     \
                    0);                                    \
(bd)->items[(bd)->count++] = dummy;                        \
}while(0)

#define hash_map_init(hm,v)  \
do                           \
{                            \
    (hm) = NULL;             \
    hmdefault((hm),(v));     \
} while (0)

#endif

